// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "EventScope.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include <gtest/gtest.h>

class EventScopeTest : public BrowserTestBase
{
protected:
	EventScopeTest() :
		m_browser1(AddBrowser()),
		m_tab1(m_browser1->AddTab()),
		m_tab2(m_browser1->AddTab()),
		m_browser2(AddBrowser()),
		m_tab3(m_browser2->AddTab())
	{
		m_browser1->ActivateTabAtIndex(0);
		m_browser2->ActivateTabAtIndex(0);
	}

	BrowserWindowFake *const m_browser1;
	Tab *const m_tab1;
	Tab *const m_tab2;

	BrowserWindowFake *const m_browser2;
	Tab *const m_tab3;
};

class EventScopeBrowserTest : public EventScopeTest
{
protected:
	using Scope = EventScope<EventScopeMinimumLevel::Browser>;
};

TEST_F(EventScopeBrowserTest, GlobalScope)
{
	auto scope = Scope::Global();
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab1));
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab2));
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab3));
}

TEST_F(EventScopeBrowserTest, BrowserScope)
{
	auto scope = Scope::ForBrowser(*m_browser1);
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab1));
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab2));
	EXPECT_FALSE(scope.DoesEventSourceMatch(*m_tab3));
}

class EventScopeShellBrowserTest : public EventScopeTest
{
protected:
	using Scope = EventScope<EventScopeMinimumLevel::ShellBrowser>;
};

TEST_F(EventScopeShellBrowserTest, GlobalScope)
{
	auto scope = Scope::Global();
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab1->GetShellBrowser()));
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab2->GetShellBrowser()));
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab3->GetShellBrowser()));
}

TEST_F(EventScopeShellBrowserTest, BrowserScope)
{
	auto scope = Scope::ForBrowser(*m_browser2);
	EXPECT_FALSE(scope.DoesEventSourceMatch(*m_tab1->GetShellBrowser()));
	EXPECT_FALSE(scope.DoesEventSourceMatch(*m_tab2->GetShellBrowser()));
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab3->GetShellBrowser()));
}

TEST_F(EventScopeShellBrowserTest, ShellBrowserScope)
{
	auto scope = Scope::ForShellBrowser(*m_tab2->GetShellBrowser());
	EXPECT_FALSE(scope.DoesEventSourceMatch(*m_tab1->GetShellBrowser()));
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab2->GetShellBrowser()));
	EXPECT_FALSE(scope.DoesEventSourceMatch(*m_tab3->GetShellBrowser()));
}

TEST_F(EventScopeShellBrowserTest, ActiveShellBrowserScope)
{
	auto scope = Scope::ForActiveShellBrowser(*m_browser1);
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab1->GetShellBrowser()));
	EXPECT_FALSE(scope.DoesEventSourceMatch(*m_tab2->GetShellBrowser()));
	EXPECT_FALSE(scope.DoesEventSourceMatch(*m_tab3->GetShellBrowser()));

	// m_tab2 is now the active tab in m_browser1, so only it should match.
	m_browser1->ActivateTabAtIndex(1);
	EXPECT_FALSE(scope.DoesEventSourceMatch(*m_tab1->GetShellBrowser()));
	EXPECT_TRUE(scope.DoesEventSourceMatch(*m_tab2->GetShellBrowser()));
	EXPECT_FALSE(scope.DoesEventSourceMatch(*m_tab3->GetShellBrowser()));
}
