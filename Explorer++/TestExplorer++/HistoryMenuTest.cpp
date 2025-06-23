// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "HistoryMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "HistoryModel.h"
#include "HistoryTracker.h"
#include "MenuViewFake.h"
#include "MenuViewFakeTestHelper.h"
#include "ShellIconLoaderFake.h"
#include <gtest/gtest.h>

class HistoryMenuTest : public BrowserTestBase
{
protected:
	HistoryMenuTest() :
		m_historyTracker(&m_historyModel, &m_navigationEvents),
		m_browser(AddBrowser()),
		m_menu(&m_menuView, &m_acceleratorManager, &m_historyModel, m_browser, &m_shellIconLoader)
	{
	}

	HistoryModel m_historyModel;
	HistoryTracker m_historyTracker;
	ShellIconLoaderFake m_shellIconLoader;

	BrowserWindowFake *const m_browser;

	MenuViewFake m_menuView;
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
	MenuViewFakeTestHelper::CheckItemDetails(&m_menuView, { pidl3, pidl2, pidl1 });

	// The menu should automatically update when the global history changes.
	PidlAbsolute pidl4;
	m_browser->AddTab(L"c:\\windows\\system32", {}, &pidl4);

	PidlAbsolute pidl5;
	m_browser->AddTab(L"e:\\", {}, &pidl5);

	MenuViewFakeTestHelper::CheckItemDetails(&m_menuView, { pidl5, pidl4, pidl3, pidl2, pidl1 });
}
