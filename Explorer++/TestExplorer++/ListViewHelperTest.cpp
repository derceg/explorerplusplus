// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ListViewHelper.h"
#include <boost/algorithm/string/predicate.hpp>
#include <gtest/gtest.h>
#include <CommCtrl.h>

class ListViewHelperTest : public testing::Test
{
protected:
	static constexpr int NUM_COLUMNS = 2;
	static inline const std::array<std::wstring, NUM_COLUMNS> COLUMNS = { L"header 1",
		L"header 2" };
	static inline const auto ITEMS = std::to_array<std::array<std::wstring, NUM_COLUMNS>>(
		{ { L"first", L"column01" }, { L"second", L"column11" }, { L"third", L"column21" } });

	void SetUp() override
	{
		m_listView.reset(CreateWindow(WC_LISTVIEW, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_listView, nullptr);

		AddColumns();
		AddItems();
	}

	wil::unique_hwnd m_listView;

private:
	void AddColumns()
	{
		int index = 0;

		for (const auto &columnText : COLUMNS)
		{
			AddColumn(index++, columnText);
		}
	}

	void AddColumn(int index, const std::wstring &text)
	{
		LV_COLUMN lvColumn = {};
		lvColumn.mask = LVCF_TEXT;
		lvColumn.pszText = const_cast<wchar_t *>(text.c_str());
		int finalIndex = ListView_InsertColumn(m_listView.get(), index, &lvColumn);
		ASSERT_EQ(finalIndex, index);
	}

	void AddItems()
	{
		int index = 0;

		for (const auto &item : ITEMS)
		{
			AddItem(index++, item);
		}
	}

	void AddItem(int index, const std::array<std::wstring, NUM_COLUMNS> &itemColumnValues)
	{
		LVITEM lvItem = {};
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = index;
		lvItem.iSubItem = 0;
		lvItem.pszText = const_cast<wchar_t *>(itemColumnValues[0].c_str());
		int finalIndex = ListView_InsertItem(m_listView.get(), &lvItem);
		ASSERT_EQ(finalIndex, index);

		for (int i = 1; i < NUM_COLUMNS; i++)
		{
			ListView_SetItemText(m_listView.get(), index, i,
				const_cast<wchar_t *>(itemColumnValues[i].c_str()));
		}
	}
};

TEST_F(ListViewHelperTest, GetItemText)
{
	for (size_t i = 0; i < ITEMS.size(); i++)
	{
		for (size_t j = 0; j < ITEMS[i].size(); j++)
		{
			EXPECT_EQ(ListViewHelper::GetItemText(m_listView.get(), static_cast<int>(i),
						  static_cast<int>(j)),
				ITEMS[i][j]);
		}
	}
}

class DoesListViewContainTextTest : public ListViewHelperTest
{
protected:
	void PerformSuccessTest(const std::wstring &text)
	{
		PerformTest(text, true);
	}

	void PerformFailureTest(const std::wstring &text)
	{
		PerformTest(text, false);
	}

private:
	void PerformTest(const std::wstring &text, bool shouldSucceed)
	{
		bool containsText = ListViewHelper::DoesListViewContainText(m_listView.get(), text,
			[](const std::wstring &input, const std::wstring &test)
			{ return boost::icontains(input, test); });
		EXPECT_EQ(containsText, shouldSucceed);
	}
};

TEST_F(DoesListViewContainTextTest, Match)
{
	PerformSuccessTest(L"eAde");
	PerformSuccessTest(L"IRst");
	PerformSuccessTest(L"mn11");
	PerformSuccessTest(L"thi");
}

TEST_F(DoesListViewContainTextTest, NoMatch)
{
	PerformFailureTest(L"header 3");
	PerformFailureTest(L"fourth");
	PerformFailureTest(L"c:\\");
	PerformFailureTest(L"column22");
}
