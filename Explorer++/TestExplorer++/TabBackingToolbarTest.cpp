// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabBackingToolbar.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "TabBackingToolbarView.h"
#include <gtest/gtest.h>

using namespace testing;

class TabBackingToolbarTest : public BrowserTestBase
{
protected:
	TabBackingToolbarTest() :
		m_browser(AddBrowser()),
		m_tabContainer(m_browser->GetActiveTabContainer()),
		m_view(m_browser->GetView(), m_browser, &m_config),
		m_toolbar(&m_view, m_browser, m_tabContainer, &m_tabEvents, &m_acceleratorManager,
			&m_resourceLoader)
	{
	}

	ToolbarButton *GetCloseButton() const
	{
		auto items = m_view.GetItems();
		CHECK_EQ(items.size(), 1u);

		const auto &closeItem = items[0];
		CHECK(closeItem.IsButton());

		return closeItem.GetButton();
	}

	BrowserWindowFake *const m_browser;
	TabContainer *const m_tabContainer;

	TabBackingToolbarView m_view;
	TabBackingToolbar m_toolbar;
};

TEST_F(TabBackingToolbarTest, CloseButtonState)
{
	Tab *tab1 = m_browser->AddTab(L"c:\\");
	Tab *tab2 = m_browser->AddTab(L"d:\\");

	tab2->SetLockState(Tab::LockState::Locked);

	m_browser->SetLifecycleState(BrowserWindow::LifecycleState::Main);

	auto *closeButton = GetCloseButton();

	m_tabContainer->SelectTab(*tab1);
	EXPECT_TRUE(closeButton->IsEnabled());

	// Closing tab1 leaves only tab2, which is locked, so the close button should be disabled.
	m_tabContainer->CloseTab(*tab1);
	tab1 = nullptr;
	EXPECT_FALSE(closeButton->IsEnabled());

	Tab *tab3 = m_browser->AddTab(L"e:\\");
	m_tabContainer->SelectTab(*tab3);
	EXPECT_TRUE(closeButton->IsEnabled());

	tab3->SetLockState(Tab::LockState::AddressLocked);
	EXPECT_FALSE(closeButton->IsEnabled());

	tab3->SetLockState(Tab::LockState::NotLocked);
	EXPECT_TRUE(closeButton->IsEnabled());
}

TEST_F(TabBackingToolbarTest, CloseButtonSelection)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	m_browser->AddTab(L"d:\\");
	int tabId3 = m_browser->AddTabAndReturnId(L"e:\\");

	m_tabContainer->SelectTabAtIndex(1);

	m_browser->SetLifecycleState(BrowserWindow::LifecycleState::Main);

	auto *closeButton = GetCloseButton();

	closeButton->OnClicked({ { 0, 0 }, false, false });
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId3)));
}
