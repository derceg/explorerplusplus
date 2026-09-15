// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "HistoryMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MenuTestHost.h"
#include "MenuViewTestHelper.h"
#include "ShellIconLoaderFake.h"
#include <gtest/gtest.h>

class HistoryMenuTest : public BrowserTestBase
{
protected:
	HistoryMenuTest() :
		m_browser(AddBrowser()),
		m_menu(m_menuHost.GetView(), &m_acceleratorManager, &m_historyModel, m_browser,
			&m_shellIconLoader)
	{
	}

	ShellIconLoaderFake m_shellIconLoader;

	BrowserWindowFake *const m_browser;

	MenuTestHost m_menuHost;
	HistoryMenu m_menu;
};

TEST_F(HistoryMenuTest, CheckItems)
{
	PidlAbsolute pidl1;
	m_browser->AddTab(L"c:\\windows", {}, &pidl1);

	PidlAbsolute pidl2;
	m_browser->AddTab(L"d:\\project\\documents", {}, &pidl2);

	PidlAbsolute pidl3;
	m_browser->AddTab(L"c:\\users", {}, &pidl3);

	// Items should appear in the reverse order that they were added to the history (i.e. with the
	// most recent item first).
	MenuViewTestHelper::CheckShellItemDetails(m_menuHost.GetView(), { pidl3, pidl2, pidl1 });

	// The menu should automatically update when the global history changes.
	PidlAbsolute pidl4;
	m_browser->AddTab(L"c:\\windows\\system32", {}, &pidl4);

	PidlAbsolute pidl5;
	m_browser->AddTab(L"e:\\", {}, &pidl5);

	MenuViewTestHelper::CheckShellItemDetails(m_menuHost.GetView(),
		{ pidl5, pidl4, pidl3, pidl2, pidl1 });
}
