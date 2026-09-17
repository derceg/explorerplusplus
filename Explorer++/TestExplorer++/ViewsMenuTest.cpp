// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ViewsMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MenuTestHost.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ViewModeHelper.h"
#include <gtest/gtest.h>

class ViewsMenuTest : public BrowserTestBase
{
protected:
	ViewsMenuTest() : m_browser(AddBrowser()), m_tab(m_browser->AddTab(L"c:\\"))
	{
	}

	void VerifyCheckState(ViewMode viewMode)
	{
		auto *shellBrowser = m_tab->GetShellBrowser();
		shellBrowser->SetViewMode(viewMode);

		MenuTestHost menuHost;
		ViewsMenu menu(menuHost.GetView(), &m_acceleratorManager, m_browser, &m_resourceLoader);

		auto *menuView = menuHost.GetView();
		ASSERT_EQ(menuView->GetNumItems(), static_cast<int>(VIEW_MODES.size()));

		for (int i = 0; i < menuView->GetNumItems(); i++)
		{
			EXPECT_EQ(menuView->IsItemChecked(menuView->GetItemIdForTesting(i)),
				VIEW_MODES[i] == viewMode);
		}
	}

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;
};

TEST_F(ViewsMenuTest, CheckState)
{
	VerifyCheckState(ViewMode::ExtraLargeIcons);
	VerifyCheckState(ViewMode::SmallIcons);
}

TEST_F(ViewsMenuTest, Selection)
{
	MenuTestHost menuHost;
	ViewsMenu menu(menuHost.GetView(), &m_acceleratorManager, m_browser, &m_resourceLoader);

	auto *menuView = menuHost.GetView();
	ASSERT_EQ(menuView->GetNumItems(), static_cast<int>(VIEW_MODES.size()));

	auto *shellBrowser = m_tab->GetShellBrowser();

	for (int i = 0; i < menuView->GetNumItems(); i++)
	{
		menuHost.SelectItemAtIndex(i, false, false);
		EXPECT_EQ(shellBrowser->GetViewMode(), VIEW_MODES[i]);
	}
}
