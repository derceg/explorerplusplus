// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SortGroupMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MenuViewTestApi.h"
#include "MenuViewTestHelper.h"
#include "RootMenuViewTestApi.h"
#include "ShellBrowserFake.h"
#include "TestMenuView.h"
#include <gtest/gtest.h>

using namespace testing;

class SortGroupMenuTestBase : public BrowserTestBase
{
protected:
	SortGroupMenuTestBase(SortGroupMenu::Mode mode) :
		m_browser(AddBrowser()),
		m_tab(m_browser->AddTab(L"c:\\")),
		m_menu(&m_menuView, &m_acceleratorManager, m_browser, mode, &m_resourceLoader)
	{
		auto *shellBrowser = GetShellBrowserFake(m_tab);

		shellBrowser->SetAvailableSortModes(
			{ SortMode::Name, SortMode::Created, SortMode::DateModified, SortMode::Size });

		if (mode == SortGroupMenu::Mode::Sort)
		{
			shellBrowser->SetSortMode(SortMode::Name);
			shellBrowser->SetSortDirection(SortDirection::Ascending);
		}
		else
		{
			shellBrowser->SetGroupMode(SortMode::Name);
			shellBrowser->SetGroupSortDirection(SortDirection::Ascending);
			shellBrowser->SetShowInGroups(true);
		}

		// The menu is only built when being shown, which is why this call is necessary.
		MenuViewTestApi::OnPopupWillShowForDpi(&m_menuView, USER_DEFAULT_SCREEN_DPI);
	}

	ShellBrowserFake *GetShellBrowserFake(Tab *tab)
	{
		auto *shellBrowserFake = dynamic_cast<ShellBrowserFake *>(tab->GetShellBrowser());
		CHECK(shellBrowserFake);
		return shellBrowserFake;
	}

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;

	TestMenuView m_menuView;
	SortGroupMenu m_menu;
};

class SortGroupMenuTest :
	public SortGroupMenuTestBase,
	public WithParamInterface<SortGroupMenu::Mode>
{
protected:
	SortGroupMenuTest() : SortGroupMenuTestBase(GetParam())
	{
	}
};

TEST_P(SortGroupMenuTest, SortDirectionStates)
{
	EXPECT_TRUE(MenuViewTestApi::IsItemEnabled(&m_menuView, SortDirection::Ascending));
	EXPECT_TRUE(MenuViewTestApi::IsItemEnabled(&m_menuView, SortDirection::Descending));

	if (GetParam() == SortGroupMenu::Mode::Group)
	{
		m_tab->GetShellBrowser()->SetShowInGroups(false);
		MenuViewTestApi::OnPopupWillShowForDpi(&m_menuView, USER_DEFAULT_SCREEN_DPI);

		// Group mode is now disabled, so the sort direction items should be disabled.
		EXPECT_FALSE(MenuViewTestApi::IsItemEnabled(&m_menuView, SortDirection::Ascending));
		EXPECT_FALSE(MenuViewTestApi::IsItemEnabled(&m_menuView, SortDirection::Descending));
	}
}

TEST_P(SortGroupMenuTest, SelectSortMode)
{
	auto *shellBrowser = m_tab->GetShellBrowser();

	// Each available sort mode should be added to the menu and selecting a sort mode should result
	// in the ShellBrowser's sort mode being updated.
	for (auto sortMode : shellBrowser->GetAvailableSortModes())
	{
		RootMenuViewTestApi::SelectItem(&m_menuView, sortMode, false, false);

		if (GetParam() == SortGroupMenu::Mode::Sort)
		{
			EXPECT_EQ(shellBrowser->GetSortMode(), sortMode);
		}
		else
		{
			EXPECT_EQ(shellBrowser->GetGroupMode(), sortMode);
		}
	}
}

TEST_P(SortGroupMenuTest, SelectCurrentSortMode)
{
	RootMenuViewTestApi::SelectItem(&m_menuView, SortMode::Name, false, false);
	auto *shellBrowser = m_tab->GetShellBrowser();

	// Selecting the current sort/group mode should result in the sort direction switching.
	if (GetParam() == SortGroupMenu::Mode::Sort)
	{
		EXPECT_EQ(shellBrowser->GetSortDirection(), SortDirection::Descending);
	}
	else
	{
		EXPECT_EQ(shellBrowser->GetGroupSortDirection(), SortDirection::Descending);
	}
}

TEST_P(SortGroupMenuTest, SelectSortDirection)
{
	RootMenuViewTestApi::SelectItem(&m_menuView, SortDirection::Descending, false, false);
	auto *shellBrowser = m_tab->GetShellBrowser();

	if (GetParam() == SortGroupMenu::Mode::Sort)
	{
		EXPECT_EQ(shellBrowser->GetSortDirection(), SortDirection::Descending);
	}
	else
	{
		EXPECT_EQ(shellBrowser->GetGroupSortDirection(), SortDirection::Descending);
	}

	RootMenuViewTestApi::SelectItem(&m_menuView, SortDirection::Ascending, false, false);

	if (GetParam() == SortGroupMenu::Mode::Sort)
	{
		EXPECT_EQ(shellBrowser->GetSortDirection(), SortDirection::Ascending);
	}
	else
	{
		EXPECT_EQ(shellBrowser->GetGroupSortDirection(), SortDirection::Ascending);
	}
}

INSTANTIATE_TEST_SUITE_P(SortAndGroupMode, SortGroupMenuTest,
	Values(SortGroupMenu::Mode::Sort, SortGroupMenu::Mode::Group));

class GroupMenuTest : public SortGroupMenuTestBase
{
protected:
	GroupMenuTest() : SortGroupMenuTestBase(SortGroupMenu::Mode::Group)
	{
	}
};

TEST_F(GroupMenuTest, SelectGroupByNone)
{
	RootMenuViewTestApi::SelectItem(&m_menuView, SortGroupMenu::Item::GroupByNone, false, false);
	EXPECT_FALSE(m_tab->GetShellBrowser()->GetShowInGroups());
}
