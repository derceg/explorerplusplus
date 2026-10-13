// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ViewsMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MenuViewTestApi.h"
#include "MenuViewTestHelper.h"
#include "RootMenuViewTestApi.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TestMenuView.h"
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

		TestMenuView menuView;
		ViewsMenu menu(&menuView, &m_acceleratorManager, m_browser, &m_resourceLoader);

		ASSERT_EQ(menuView.GetNumItems(), static_cast<int>(VIEW_MODES.size()));

		for (int i = 0; i < menuView.GetNumItems(); i++)
		{
			EXPECT_EQ(
				MenuViewTestApi::IsItemChecked(&menuView, MenuViewTestApi::GetItemId(&menuView, i)),
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
	TestMenuView menuView;
	ViewsMenu menu(&menuView, &m_acceleratorManager, m_browser, &m_resourceLoader);

	ASSERT_EQ(menuView.GetNumItems(), static_cast<int>(VIEW_MODES.size()));

	auto *shellBrowser = m_tab->GetShellBrowser();

	for (int i = 0; i < menuView.GetNumItems(); i++)
	{
		RootMenuViewTestApi::SelectItemAtIndex(&menuView, i, false, false);
		EXPECT_EQ(shellBrowser->GetViewMode(), VIEW_MODES[i]);
	}
}
