// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "AddressBar.h"
#include "AddressBarView.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "PidlTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "Tab.h"
#include <gtest/gtest.h>
#include <memory>

using namespace testing;

class AddressBarTest : public BrowserTestBase
{
protected:
	AddressBarTest() :
		m_browser(AddBrowser()),
		m_addressBarView(std::make_unique<AddressBarView>(m_browser->GetHWND(), &m_config)),
		m_addressBar(std::make_unique<AddressBar>(m_addressBarView.get(), m_browser, &m_tabEvents,
			&m_shellBrowserEvents, &m_navigationEvents, &m_runtime, &m_iconFetcher))
	{
	}

	BrowserWindowFake *const m_browser;
	std::unique_ptr<AddressBarView> m_addressBarView;
	std::unique_ptr<AddressBar> m_addressBar;
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
	m_browser->GetActiveTabBar()->SelectTabAtIndex(1);
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
	EXPECT_NE(commandTargetManager->GetCurrentTarget(), m_addressBar.get());

	// When the address bar receives focus, it should mark itself as the active target.
	auto *delegate = m_addressBarView->GetDelegateForTesting();
	delegate->OnFocused();
	EXPECT_EQ(commandTargetManager->GetCurrentTarget(), m_addressBar.get());
}

TEST_F(AddressBarTest, IconRetrievalAfterDestruction)
{
	// Doing this will cause the address bar to queue at least one icon retrieval task.
	m_browser->AddTab(L"c:\\");
	EXPECT_TRUE(m_iconFetcher.HasPendingRequests());

	m_addressBar.reset();
	m_addressBarView.reset();

	// This will cause the pending icon retrieval task(s) to return. This should be safe, even
	// though the address bar has been destroyed.
	m_iconFetcher.CompletePendingRequests();
}
