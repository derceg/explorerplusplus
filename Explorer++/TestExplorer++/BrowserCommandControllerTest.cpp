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
#include <gtest/gtest.h>

using namespace testing;

class BrowserCommandControllerTest : public BrowserTestBase
{
protected:
	BrowserCommandControllerTest() :
		m_browser(AddBrowser()),
		m_tab(m_browser->AddTab()),
		m_commandController(m_browser, &m_config)
	{
		m_browser->ActivateTabAtIndex(0);
	}

	void NavigateTab(Tab *tab, const std::wstring &path)
	{
		auto pidl = CreateSimplePidlForTest(path);
		auto navigateParams = NavigateParams::Normal(pidl.Raw());
		tab->GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
	}

	Config m_config;

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;
	BrowserCommandController m_commandController;
};

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
