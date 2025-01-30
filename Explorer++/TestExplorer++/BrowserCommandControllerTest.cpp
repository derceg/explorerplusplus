// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserCommandController.h"
#include "MainResource.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellTestHelper.h"
#include "TabNavigationMock.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class BrowserCommandControllerTest : public Test
{
protected:
	BrowserCommandControllerTest() :
		m_shellBrowser(&m_tabNavigation),
		m_commandController(&m_shellBrowser)
	{
	}

	TabNavigationMock m_tabNavigation;
	ShellBrowserFake m_shellBrowser;
	BrowserCommandController m_commandController;
};

TEST_F(BrowserCommandControllerTest, Back)
{
	m_shellBrowser.NavigateToPath(L"C:\\Fake1");
	m_shellBrowser.NavigateToPath(L"C:\\Fake2");
	m_shellBrowser.NavigateToPath(L"C:\\Fake3");

	m_commandController.ExecuteCommand(IDM_GO_BACK);
	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 1);
}

TEST_F(BrowserCommandControllerTest, Forward)
{
	m_shellBrowser.NavigateToPath(L"C:\\Fake1");
	m_shellBrowser.NavigateToPath(L"C:\\Fake2");
	m_shellBrowser.NavigateToPath(L"C:\\Fake3");

	m_shellBrowser.GetNavigationController()->GoBack();
	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 1);

	m_commandController.ExecuteCommand(IDM_GO_FORWARD);
	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 2);
}

TEST_F(BrowserCommandControllerTest, Up)
{
	m_shellBrowser.NavigateToPath(L"C:\\Fake");

	m_commandController.ExecuteCommand(IDM_GO_UP);
	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetNumHistoryEntries(), 2);

	auto *currentEntry = m_shellBrowser.GetNavigationController()->GetCurrentEntry();
	ASSERT_NE(currentEntry, nullptr);

	PidlAbsolute pidlParent = CreateSimplePidlForTest(L"C:\\");
	EXPECT_EQ(currentEntry->GetPidl(), pidlParent);
}
