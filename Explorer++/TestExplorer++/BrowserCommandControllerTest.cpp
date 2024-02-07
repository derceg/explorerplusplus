// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserCommandController.h"
#include "IconFetcherMock.h"
#include "MainResource.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellNavigatorMock.h"
#include "TabNavigationMock.h"
#include <gtest/gtest.h>

using namespace testing;

class BrowserCommandControllerTest : public Test
{
protected:
	BrowserCommandControllerTest() :
		m_navigationController(&m_navigator, &m_tabNavigation, &m_iconFetcher),
		m_shellBrowser(&m_navigationController),
		m_commandController(&m_shellBrowser)
	{
	}

	ShellNavigatorMock m_navigator;
	TabNavigationMock m_tabNavigation;
	IconFetcherMock m_iconFetcher;
	ShellNavigationController m_navigationController;
	ShellBrowserFake m_shellBrowser;
	BrowserCommandController m_commandController;
};

TEST_F(BrowserCommandControllerTest, Back)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToFolder(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToFolder(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToFolder(L"C:\\Fake3"));

	m_commandController.ExecuteCommand(IDM_GO_BACK);
	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 1);
}

TEST_F(BrowserCommandControllerTest, Forward)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToFolder(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToFolder(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToFolder(L"C:\\Fake3"));

	m_shellBrowser.GetNavigationController()->GoBack();
	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 1);

	m_commandController.ExecuteCommand(IDM_GO_FORWARD);
	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 2);
}

TEST_F(BrowserCommandControllerTest, Up)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToFolder(L"C:\\Fake"));

	m_commandController.ExecuteCommand(IDM_GO_UP);
	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetNumHistoryEntries(), 2);

	auto *currentEntry = m_shellBrowser.GetNavigationController()->GetCurrentEntry();
	ASSERT_NE(currentEntry, nullptr);

	unique_pidl_absolute pidlParent(SHSimpleIDListFromPath(L"C:\\"));
	EXPECT_TRUE(ArePidlsEquivalent(currentEntry->GetPidl().get(), pidlParent.get()));
}
