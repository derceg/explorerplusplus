// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "GlobalHistoryMenu.h"
#include "BrowserWindowMock.h"
#include "HistoryService.h"
#include "IconFetcherMock.h"
#include "PopupMenuView.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class GlobalHistoryMenuTest : public Test
{
protected:
	GlobalHistoryMenuTest() :
		m_menu(&m_popupMenu, &m_historyService, &m_browserWindow, &m_iconFetcher)
	{
	}

	void AddHistoryItems(size_t count)
	{
		for (size_t i = 0; i < count; i++)
		{
			AddHistoryItem();
		}
	}

	void CheckItemDetails()
	{
		ASSERT_EQ(static_cast<size_t>(m_popupMenu.GetItemCountForTesting()), m_historyItemCount);

		for (size_t i = 0; i < m_historyItemCount; i++)
		{
			// Items should appear in the reverse order that they were added to the history (i.e.
			// with the most recent item first).
			auto id = m_popupMenu.GetItemIdForTesting(static_cast<int>(m_historyItemCount - i - 1));

			auto name = GetNameForItem(i);
			EXPECT_EQ(m_popupMenu.GetItemTextForTesting(id), name);
			EXPECT_EQ(m_popupMenu.GetHelpTextForItem(id), GetPathForItem(name));
		}
	}

private:
	std::wstring GetNameForItem(size_t index)
	{
		return L"Fake" + std::to_wstring(index);
	}

	std::wstring GetPathForItem(const std::wstring &name)
	{
		return L"C:\\" + name;
	}

	void AddHistoryItem()
	{
		auto path = GetPathForItem(GetNameForItem(m_historyItemCount));
		unique_pidl_absolute pidl(SHSimpleIDListFromPath(path.c_str()));
		ASSERT_NE(pidl, nullptr);
		m_historyService.AddHistoryItem(pidl.get());

		m_historyItemCount++;
	}

	PopupMenuView m_popupMenu;
	HistoryService m_historyService;
	BrowserWindowMock m_browserWindow;
	IconFetcherMock m_iconFetcher;
	GlobalHistoryMenu m_menu;
	size_t m_historyItemCount = 0;
};

TEST_F(GlobalHistoryMenuTest, CheckItems)
{
	AddHistoryItems(3);
	CheckItemDetails();
}

TEST_F(GlobalHistoryMenuTest, UpdateHistory)
{
	AddHistoryItems(3);

	// The menu should automatically update when the global history changes.
	AddHistoryItems(3);
	CheckItemDetails();
}
