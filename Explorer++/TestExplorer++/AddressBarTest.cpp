// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "AddressBar.h"
#include "AddressBarView.h"
#include "AsyncIconFetcher.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "PidlTestHelper.h"
#include "Runtime.h"
#include "RuntimeTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "Tab.h"
#include "../Helper/CachedIcons.h"
#include <gtest/gtest.h>
#include <memory>

using namespace testing;

class AddressBarTest : public BrowserTestBase
{
protected:
	AddressBarTest() :
		m_runtime(BuildRuntimeForTest()),
		m_cachedIcons(std::make_shared<CachedIcons>(10)),
		m_iconFetcher(std::make_shared<AsyncIconFetcher>(&m_runtime, m_cachedIcons)),
		m_browser(AddBrowser()),
		m_addressBarView(AddressBarView::Create(m_browser->GetHWND(), &m_config)),
		m_addressBar(AddressBar::Create(m_addressBarView, m_browser, &m_tabEvents,
			&m_shellBrowserEvents, &m_navigationEvents, &m_runtime, m_iconFetcher))
	{
	}

	Runtime m_runtime;
	std::shared_ptr<CachedIcons> m_cachedIcons;
	std::shared_ptr<AsyncIconFetcher> m_iconFetcher;

	BrowserWindowFake *const m_browser;
	AddressBarView *const m_addressBarView;
	AddressBar *const m_addressBar;
};

TEST_F(AddressBarTest, DisplayUpdateAfterNavigation)
{
	auto *tab1 = m_browser->AddTab(L"c:\\");
	auto *tab2 = m_browser->AddTab(L"c:\\");

	auto *browser2 = AddBrowser();
	auto *tab3 = browser2->AddTab(L"c:\\");

	std::wstring path = L"c:\\path\\to\\folder";
	NavigateTab(tab1, path);
	EXPECT_THAT(m_addressBarView->GetText(), StrCaseEq(path));

	// tab2 isn't active, so navigations in it should be ignored by the address bar. The path shown
	// should still be the original path.
	NavigateTab(tab2, L"d:\\");
	EXPECT_THAT(m_addressBarView->GetText(), StrCaseEq(path));

	// tab3 is in a different browser window, so navigations in it should also be ignored.
	NavigateTab(tab3, L"e:\\");
	EXPECT_THAT(m_addressBarView->GetText(), StrCaseEq(path));
}

TEST_F(AddressBarTest, DisplayUpdateAfterTabSwitch)
{
	std::wstring path1 = L"c:\\path1";
	m_browser->AddTab(path1);

	std::wstring path2 = L"c:\\path2";
	m_browser->AddTab(path2);

	EXPECT_THAT(m_addressBarView->GetText(), StrCaseEq(path1));
	m_browser->GetActiveTabContainer()->SelectTabAtIndex(1);
	EXPECT_THAT(m_addressBarView->GetText(), StrCaseEq(path2));
}

TEST_F(AddressBarTest, OpenItemOnEnter)
{
	auto *tab1 = m_browser->AddTab(L"c:\\original\\path");

	std::wstring updatedPath = L"c:\\updated\\path";
	m_addressBarView->SetTextForTesting(updatedPath);
	auto *delegate = m_addressBarView->GetDelegateForTesting();
	delegate->OnKeyPressed(VK_RETURN);

	// The key press above should have resulted in a navigation in the active tab.
	EXPECT_EQ(tab1->GetShellBrowser()->GetNavigationController()->GetNumHistoryEntries(), 2);

	auto *currentEntry = tab1->GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
	ASSERT_NE(currentEntry, nullptr);
	EXPECT_EQ(currentEntry->GetPidl(), CreateSimplePidlForTest(updatedPath));
}

TEST_F(AddressBarTest, RevertTextOnEscape)
{
	std::wstring path = L"c:\\path";
	m_browser->AddTab(path);

	m_addressBarView->SetTextForTesting(L"c:\\path\\to\\second\\folder");

	// This should result in the text being reverted back to the original text.
	auto *delegate = m_addressBarView->GetDelegateForTesting();
	delegate->OnKeyPressed(VK_ESCAPE);
	EXPECT_THAT(m_addressBarView->GetText(), StrCaseEq(path));
}

TEST_F(AddressBarTest, ActiveTargetOnFocus)
{
	auto *commandTargetManager = m_browser->GetCommandTargetManager();
	EXPECT_NE(commandTargetManager->GetCurrentTarget(), m_addressBar);

	// When the address bar receives focus, it should mark itself as the active target.
	auto *delegate = m_addressBarView->GetDelegateForTesting();
	delegate->OnFocused();
	EXPECT_EQ(commandTargetManager->GetCurrentTarget(), m_addressBar);
}
