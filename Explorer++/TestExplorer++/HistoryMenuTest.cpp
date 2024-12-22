// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "HistoryMenu.h"
#include "AcceleratorManager.h"
#include "BrowserWindowMock.h"
#include "HistoryModel.h"
#include "PopupMenuView.h"
#include "PopupMenuViewTestHelper.h"
#include "ShellIconLoaderFake.h"
#include "ShellTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class HistoryMenuTest : public Test
{
protected:
	HistoryMenuTest() :
		m_menu(&m_popupMenu, &m_acceleratorManager, &m_historyModel, &m_browserWindow,
			&m_shellIconLoader)
	{
	}

	PopupMenuView m_popupMenu;
	AcceleratorManager m_acceleratorManager;
	HistoryModel m_historyModel;
	BrowserWindowMock m_browserWindow;
	ShellIconLoaderFake m_shellIconLoader;
	HistoryMenu m_menu;
};

TEST_F(HistoryMenuTest, CheckItems)
{
	auto pidl1 = CreateSimplePidlForTest(L"c:\\windows");
	m_historyModel.AddHistoryItem(pidl1);

	auto pidl2 = CreateSimplePidlForTest(L"d:\\project\\documents");
	m_historyModel.AddHistoryItem(pidl2);

	auto pidl3 = CreateSimplePidlForTest(L"c:\\users");
	m_historyModel.AddHistoryItem(pidl3);

	// Items should appear in the reverse order that they were added to the history (i.e. with the
	// most recent item first).
	PopupMenuViewTestHelper::CheckItemDetails(&m_popupMenu, { pidl3, pidl2, pidl1 });

	// The menu should automatically update when the global history changes.
	auto pidl4 = CreateSimplePidlForTest(L"c:\\windows\\system32");
	m_historyModel.AddHistoryItem(pidl4);

	auto pidl5 = CreateSimplePidlForTest(L"e:\\");
	m_historyModel.AddHistoryItem(pidl5);

	PopupMenuViewTestHelper::CheckItemDetails(&m_popupMenu, { pidl5, pidl4, pidl3, pidl2, pidl1 });
}
