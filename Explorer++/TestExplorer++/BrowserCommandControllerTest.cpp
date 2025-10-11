// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserCommandController.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include "../Helper/Clipboard.h"
#include <gtest/gtest.h>

using namespace testing;

class BrowserCommandControllerTest : public BrowserTestBase
{
protected:
	BrowserCommandControllerTest() :
		m_browser(AddBrowser()),
		m_originalPath(L"c:\\"),
		m_tab(m_browser->AddTab(m_originalPath)),
		m_commandController(m_browser, &m_config, m_platformContext.GetClipboardStore(),
			&m_resourceLoader)
	{
	}

	BrowserWindowFake *const m_browser;
	const std::wstring m_originalPath;
	Tab *const m_tab;
	BrowserCommandController m_commandController;
};

TEST_F(BrowserCommandControllerTest, SortBy)
{
	m_commandController.ExecuteCommand(IDM_SORTBY_ATTRIBUTES);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetSortMode(), +SortMode::Attributes);

	m_tab->GetShellBrowser()->SetSortDirection(SortDirection::Ascending);

	// Selecting the same sort mode again should result in the sort direction being flipped. The
	// sort mode itself shouldn't change.
	m_commandController.ExecuteCommand(IDM_SORTBY_ATTRIBUTES);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetSortMode(), +SortMode::Attributes);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetSortDirection(), +SortDirection::Descending);

	m_commandController.ExecuteCommand(IDM_SORTBY_ATTRIBUTES);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetSortMode(), +SortMode::Attributes);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetSortDirection(), +SortDirection::Ascending);
}

TEST_F(BrowserCommandControllerTest, CloseTab)
{
	int tabId2 = m_browser->AddTabAndReturnId(L"c:\\");

	auto *tabContainer = m_browser->GetActiveTabContainer();
	tabContainer->SelectTab(tabContainer->GetTab(tabId2));

	m_commandController.ExecuteCommand(IDM_FILE_CLOSETAB);
	EXPECT_EQ(tabContainer->GetNumTabs(), 1);
	EXPECT_EQ(tabContainer->MaybeGetTab(tabId2), nullptr);
}

TEST_F(BrowserCommandControllerTest, CopyFolderPath)
{
	std::wstring path = L"c:\\fake";
	NavigateTab(m_tab, path);

	m_commandController.ExecuteCommand(IDM_FILE_COPYFOLDERPATH);

	Clipboard clipboard(m_platformContext.GetClipboardStore());
	auto clipboardText = clipboard.ReadText();
	ASSERT_TRUE(clipboardText.has_value());
	EXPECT_THAT(*clipboardText, StrCaseEq(path));
}

TEST_F(BrowserCommandControllerTest, CanChangeMainFontSize)
{
	m_config.mainFont = std::nullopt;
	EXPECT_TRUE(m_commandController.IsCommandEnabled(IDM_VIEW_DECREASE_TEXT_SIZE));
	EXPECT_TRUE(m_commandController.IsCommandEnabled(IDM_VIEW_INCREASE_TEXT_SIZE));

	m_config.mainFont = CustomFont(L"Font name", CustomFont::MINIMUM_SIZE + 1);
	EXPECT_TRUE(m_commandController.IsCommandEnabled(IDM_VIEW_DECREASE_TEXT_SIZE));
	EXPECT_TRUE(m_commandController.IsCommandEnabled(IDM_VIEW_INCREASE_TEXT_SIZE));

	m_config.mainFont = CustomFont(L"Font name", CustomFont::MINIMUM_SIZE);
	EXPECT_FALSE(m_commandController.IsCommandEnabled(IDM_VIEW_DECREASE_TEXT_SIZE));
	EXPECT_TRUE(m_commandController.IsCommandEnabled(IDM_VIEW_INCREASE_TEXT_SIZE));

	m_config.mainFont = CustomFont(L"Font name", CustomFont::MAXIMUM_SIZE);
	EXPECT_TRUE(m_commandController.IsCommandEnabled(IDM_VIEW_DECREASE_TEXT_SIZE));
	EXPECT_FALSE(m_commandController.IsCommandEnabled(IDM_VIEW_INCREASE_TEXT_SIZE));
}

TEST_F(BrowserCommandControllerTest, ChangeMainFontSize)
{
	const std::wstring fontName = L"Font name";
	m_config.mainFont = CustomFont(fontName, 10);

	m_commandController.ExecuteCommand(IDM_VIEW_DECREASE_TEXT_SIZE);
	EXPECT_EQ(m_config.mainFont.get(), CustomFont(fontName, 9));

	m_commandController.ExecuteCommand(IDM_VIEW_INCREASE_TEXT_SIZE);
	EXPECT_EQ(m_config.mainFont.get(), CustomFont(fontName, 10));

	// If no custom font is currently set and a font size change is requested, a custom font should
	// be assigned.
	m_config.mainFont = std::nullopt;
	m_commandController.ExecuteCommand(IDM_VIEW_DECREASE_TEXT_SIZE);
	EXPECT_NE(m_config.mainFont.get(), std::nullopt);

	m_config.mainFont = std::nullopt;
	m_commandController.ExecuteCommand(IDM_VIEW_INCREASE_TEXT_SIZE);
	EXPECT_NE(m_config.mainFont.get(), std::nullopt);
}

TEST_F(BrowserCommandControllerTest, ResetMainFontSize)
{
	// If there is no custom font assigned, resetting the font size should have no effect.
	m_config.mainFont = std::nullopt;
	m_commandController.ExecuteCommand(IDA_RESET_TEXT_SIZE);
	EXPECT_EQ(m_config.mainFont.get(), std::nullopt);

	const std::wstring fontName = L"Font name";
	m_config.mainFont = CustomFont(fontName, 10);

	// Resetting the font size shouldn't change the name of the font.
	m_commandController.ExecuteCommand(IDA_RESET_TEXT_SIZE);
	const auto &mainFont = m_config.mainFont.get();
	ASSERT_NE(mainFont, std::nullopt);
	EXPECT_EQ(mainFont->GetName(), fontName);
}

TEST_F(BrowserCommandControllerTest, Refresh)
{
	auto *navigationController = m_tab->GetShellBrowser()->GetNavigationController();

	int originalEntryId = navigationController->GetCurrentEntry()->GetId();
	auto originalPidl = navigationController->GetCurrentEntry()->GetPidl();

	m_commandController.ExecuteCommand(IDM_VIEW_REFRESH);

	// Refreshing should result in the current entry being replaced, but shouldn't add any entries.
	ASSERT_EQ(navigationController->GetNumHistoryEntries(), 1);

	auto *updatedEntry = navigationController->GetCurrentEntry();
	EXPECT_NE(updatedEntry->GetId(), originalEntryId);
	EXPECT_EQ(updatedEntry->GetPidl(), originalPidl);
}

TEST_F(BrowserCommandControllerTest, Back)
{
	NavigateTab(m_tab, L"c:\\fake");

	m_commandController.ExecuteCommand(IDM_GO_BACK);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 0);
}

TEST_F(BrowserCommandControllerTest, BackInNewTab)
{
	NavigateTab(m_tab, L"c:\\fake");

	m_commandController.ExecuteCommand(IDM_GO_BACK, OpenFolderDisposition::ForegroundTab);

	// The navigation should occur in a new tab, so the current tab should remain in the same
	// folder.
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 1);

	auto *tabContainer = m_browser->GetActiveTabContainer();
	ASSERT_EQ(tabContainer->GetNumTabs(), 2);
	EXPECT_EQ(tabContainer->GetTabByIndex(1).GetShellBrowser()->GetDirectory(),
		CreateSimplePidlForTest(m_originalPath));
}

TEST_F(BrowserCommandControllerTest, Forward)
{
	NavigateTab(m_tab, L"c:\\fake");

	m_tab->GetShellBrowser()->GetNavigationController()->GoBack();

	m_commandController.ExecuteCommand(IDM_GO_FORWARD);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 1);
}

TEST_F(BrowserCommandControllerTest, ForwardInNewTab)
{
	std::wstring path = L"c:\\fake";
	NavigateTab(m_tab, path);

	m_tab->GetShellBrowser()->GetNavigationController()->GoBack();

	m_commandController.ExecuteCommand(IDM_GO_FORWARD, OpenFolderDisposition::ForegroundTab);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 0);

	auto *tabContainer = m_browser->GetActiveTabContainer();
	ASSERT_EQ(tabContainer->GetNumTabs(), 2);
	EXPECT_EQ(tabContainer->GetTabByIndex(1).GetShellBrowser()->GetDirectory(),
		CreateSimplePidlForTest(path));
}

TEST_F(BrowserCommandControllerTest, Up)
{
	NavigateTab(m_tab, L"c:\\windows\\system32");

	m_commandController.ExecuteCommand(IDM_GO_UP);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetNumHistoryEntries(), 3);

	auto *currentEntry = m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
	ASSERT_NE(currentEntry, nullptr);

	PidlAbsolute pidlParent = CreateSimplePidlForTest(L"c:\\windows");
	EXPECT_EQ(currentEntry->GetPidl(), pidlParent);
}

TEST_F(BrowserCommandControllerTest, UpInNewTab)
{
	NavigateTab(m_tab, L"c:\\windows\\system32");

	m_commandController.ExecuteCommand(IDM_GO_UP, OpenFolderDisposition::ForegroundTab);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetNumHistoryEntries(), 2);

	auto *tabContainer = m_browser->GetActiveTabContainer();
	ASSERT_EQ(tabContainer->GetNumTabs(), 2);
	EXPECT_EQ(tabContainer->GetTabByIndex(1).GetShellBrowser()->GetDirectory(),
		CreateSimplePidlForTest(L"c:\\windows"));
}

TEST_F(BrowserCommandControllerTest, SelectAdjacentTab)
{
	m_browser->AddTab(L"c:\\");

	m_commandController.ExecuteCommand(IDA_SELECT_NEXT_TAB);
	auto *tabContainer = m_browser->GetActiveTabContainer();
	EXPECT_EQ(tabContainer->GetSelectedTabIndex(), 1);

	m_commandController.ExecuteCommand(IDA_SELECT_PREVIOUS_TAB);
	EXPECT_EQ(tabContainer->GetSelectedTabIndex(), 0);
}

TEST_F(BrowserCommandControllerTest, DuplicateTab)
{
	PidlAbsolute pidl;
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\project", {}, &pidl);

	auto *tabContainer = m_browser->GetActiveTabContainer();
	tabContainer->SelectTab(tabContainer->GetTab(tabId2));

	m_commandController.ExecuteCommand(IDA_DUPLICATE_TAB);
	ASSERT_EQ(tabContainer->GetNumTabs(), 3);

	const auto &duplicatedTab = tabContainer->GetTabByIndex(2);
	EXPECT_NE(duplicatedTab.GetId(), tabId2);
	EXPECT_EQ(duplicatedTab.GetShellBrowser()->GetDirectory(), pidl);
}

TEST_F(BrowserCommandControllerTest, SelectTabAtIndex)
{
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"c:\\");

	m_commandController.ExecuteCommand(IDA_SELECT_TAB_4);
	EXPECT_EQ(m_browser->GetActiveTabContainer()->GetSelectedTabIndex(), 3);
}

TEST_F(BrowserCommandControllerTest, SelectTabAtIndexPastEnd)
{
	m_browser->AddTab(L"c:\\");

	// Attempting to select a tab past the last tab should result in the last tab being selected.
	m_commandController.ExecuteCommand(IDA_SELECT_TAB_7);
	EXPECT_EQ(m_browser->GetActiveTabContainer()->GetSelectedTabIndex(), 1);
}

TEST_F(BrowserCommandControllerTest, SelectLastTab)
{
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"c:\\");

	m_commandController.ExecuteCommand(IDA_SELECT_LAST_TAB);
	EXPECT_EQ(m_browser->GetActiveTabContainer()->GetSelectedTabIndex(), 3);
}
