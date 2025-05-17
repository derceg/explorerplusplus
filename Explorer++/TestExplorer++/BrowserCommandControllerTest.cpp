// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserCommandController.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "Config.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include "SimulatedClipboardStore.h"
#include "../Helper/Clipboard.h"
#include <gtest/gtest.h>

using namespace testing;

class BrowserCommandControllerTest : public BrowserTestBase
{
protected:
	BrowserCommandControllerTest() :
		m_browser(AddBrowser()),
		m_tab(m_browser->AddTab()),
		m_commandController(m_browser, &m_config, &m_clipboardStore)
	{
		m_browser->ActivateTabAtIndex(0);
	}

	void NavigateTab(Tab *tab, const std::wstring &path, PidlAbsolute *outputPidl = nullptr)
	{
		auto pidl = CreateSimplePidlForTest(path);
		auto navigateParams = NavigateParams::Normal(pidl.Raw());
		tab->GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);

		wil::assign_to_opt_param(outputPidl, pidl);
	}

	Config m_config;
	SimulatedClipboardStore m_clipboardStore;

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;
	BrowserCommandController m_commandController;
};

TEST_F(BrowserCommandControllerTest, CopyFolderPath)
{
	std::wstring path = L"C:\\Fake";
	NavigateTab(m_tab, path);

	m_commandController.ExecuteCommand(IDM_FILE_COPYFOLDERPATH);

	Clipboard clipboard(&m_clipboardStore);
	auto clipboardText = clipboard.ReadText();
	ASSERT_TRUE(clipboardText.has_value());
	EXPECT_THAT(*clipboardText, StrCaseEq(path));
}

TEST_F(BrowserCommandControllerTest, Refresh)
{
	PidlAbsolute pidl;
	NavigateTab(m_tab, L"C:\\Fake", &pidl);

	auto *navigationController = m_tab->GetShellBrowser()->GetNavigationController();

	int originalEntryId = navigationController->GetCurrentEntry()->GetId();

	m_commandController.ExecuteCommand(IDM_VIEW_REFRESH);

	// Refreshing should result in the current entry being replaced, but shouldn't add any entries.
	ASSERT_EQ(navigationController->GetNumHistoryEntries(), 1);

	auto *updatedEntry = navigationController->GetCurrentEntry();
	EXPECT_NE(updatedEntry->GetId(), originalEntryId);
	EXPECT_EQ(updatedEntry->GetPidl(), pidl);
}

TEST_F(BrowserCommandControllerTest, Back)
{
	NavigateTab(m_tab, L"C:\\Fake1");
	NavigateTab(m_tab, L"C:\\Fake2");
	NavigateTab(m_tab, L"C:\\Fake3");

	m_commandController.ExecuteCommand(IDM_GO_BACK);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 1);
}

TEST_F(BrowserCommandControllerTest, Forward)
{
	NavigateTab(m_tab, L"C:\\Fake1");
	NavigateTab(m_tab, L"C:\\Fake2");
	NavigateTab(m_tab, L"C:\\Fake3");

	m_tab->GetShellBrowser()->GetNavigationController()->GoBack();
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 1);

	m_commandController.ExecuteCommand(IDM_GO_FORWARD);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 2);
}

TEST_F(BrowserCommandControllerTest, Up)
{
	NavigateTab(m_tab, L"C:\\Fake");

	m_commandController.ExecuteCommand(IDM_GO_UP);
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetNumHistoryEntries(), 2);

	auto *currentEntry = m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
	ASSERT_NE(currentEntry, nullptr);

	PidlAbsolute pidlParent = CreateSimplePidlForTest(L"C:\\");
	EXPECT_EQ(currentEntry->GetPidl(), pidlParent);
}
