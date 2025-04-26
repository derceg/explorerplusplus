// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ListViewHelper.h"
#include <boost/algorithm/string/predicate.hpp>
#include <gtest/gtest.h>
#include <CommCtrl.h>

using namespace testing;

class ListViewHelperTest : public Test
{
protected:
	static constexpr int NUM_COLUMNS = 2;
	using RowData = std::array<const wchar_t *, NUM_COLUMNS>;
	static constexpr RowData COLUMNS = { L"header 1", L"header 2" };
	static constexpr auto ITEMS = std::to_array<RowData>(
		{ { L"first", L"column01" }, { L"second", L"column11" }, { L"third", L"column21" } });

	void SetUp() override
	{
		m_listView.reset(CreateWindow(WC_LISTVIEW, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_listView, nullptr);

		AddColumns();
		AddItems();
	}

	// Checks that the text data contained within the listview (in each row and column) matches the
	// provided data.
	void CheckListViewItemData(const std::array<RowData, ITEMS.size()> &expectedItemData)
	{
		for (size_t i = 0; i < expectedItemData.size(); i++)
		{
			for (size_t j = 0; j < expectedItemData[i].size(); j++)
			{
				EXPECT_EQ(ListViewHelper::GetItemText(m_listView.get(), static_cast<int>(i),
							  static_cast<int>(j)),
					expectedItemData[i][j]);
			}
		}
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

	void AddItem(int index, const RowData &itemRowData)
	{
		LVITEM lvItem = {};
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = index;
		lvItem.iSubItem = 0;
		lvItem.pszText = const_cast<wchar_t *>(itemRowData[0]);
		int finalIndex = ListView_InsertItem(m_listView.get(), &lvItem);
		ASSERT_EQ(finalIndex, index);

		for (int i = 1; i < NUM_COLUMNS; i++)
		{
			ListView_SetItemText(m_listView.get(), index, i, const_cast<wchar_t *>(itemRowData[i]));
		}
	}
};

TEST_F(ListViewHelperTest, GetItemText)
{
	CheckListViewItemData(ITEMS);
}

TEST_F(ListViewHelperTest, GetItemTextLong)
{
	std::wstring text(1000, 'a');
	ListView_SetItemText(m_listView.get(), 0, 0, const_cast<wchar_t *>(text.c_str()));

	auto retrievedText = ListViewHelper::GetItemText(m_listView.get(), 0, 0);
	EXPECT_EQ(retrievedText, text);
}

TEST_F(ListViewHelperTest, AddRemoveExtendedStyles)
{
	constexpr DWORD style = LVS_EX_CHECKBOXES;

	DWORD extendedStyle = ListView_GetExtendedListViewStyle(m_listView.get());
	DWORD originalExtendedStyle = extendedStyle;
	ASSERT_FALSE(WI_IsFlagSet(extendedStyle, style));

	ListViewHelper::AddRemoveExtendedStyles(m_listView.get(), style, true);
	extendedStyle = ListView_GetExtendedListViewStyle(m_listView.get());
	EXPECT_TRUE(WI_IsFlagSet(extendedStyle, style));

	ListViewHelper::AddRemoveExtendedStyles(m_listView.get(), style, false);
	extendedStyle = ListView_GetExtendedListViewStyle(m_listView.get());
	EXPECT_FALSE(WI_IsFlagSet(extendedStyle, style));
	EXPECT_EQ(extendedStyle, originalExtendedStyle);
}

TEST_F(ListViewHelperTest, SwapItems)
{
	ListViewHelper::SelectItem(m_listView.get(), 2, true);
	ListViewHelper::SwapItems(m_listView.get(), 0, 2);

	auto updatedItems = ITEMS;
	std::swap(updatedItems[0], updatedItems[2]);
	CheckListViewItemData(updatedItems);

	// Item 2 was selected, then swapped with item 0. So item 0 should now be selected.
	EXPECT_THAT(ListViewHelper::GetSelectedItems(m_listView.get()), ElementsAre(0));
}

TEST_F(ListViewHelperTest, GetSelectedItems)
{
	ListViewHelper::SelectItem(m_listView.get(), 1, true);
	ListViewHelper::SelectItem(m_listView.get(), 2, true);
	EXPECT_THAT(ListViewHelper::GetSelectedItems(m_listView.get()), ElementsAre(1, 2));

	ListViewHelper::SelectItem(m_listView.get(), 2, false);
	EXPECT_THAT(ListViewHelper::GetSelectedItems(m_listView.get()), ElementsAre(1));

	ListViewHelper::SelectItem(m_listView.get(), 0, true);
	EXPECT_THAT(ListViewHelper::GetSelectedItems(m_listView.get()), ElementsAre(0, 1));
}

TEST_F(ListViewHelperTest, SelectAllItems)
{
	ListViewHelper::SelectAllItems(m_listView.get(), true);
	EXPECT_THAT(ListViewHelper::GetSelectedItems(m_listView.get()), ElementsAre(0, 1, 2));

	ListViewHelper::SelectAllItems(m_listView.get(), false);
	EXPECT_THAT(ListViewHelper::GetSelectedItems(m_listView.get()), IsEmpty());
}

TEST_F(ListViewHelperTest, InvertSelection)
{
	ListViewHelper::SelectItem(m_listView.get(), 1, true);
	EXPECT_THAT(ListViewHelper::GetSelectedItems(m_listView.get()), ElementsAre(1));

	ListViewHelper::InvertSelection(m_listView.get());
	EXPECT_THAT(ListViewHelper::GetSelectedItems(m_listView.get()), ElementsAre(0, 2));

	ListViewHelper::InvertSelection(m_listView.get());
	EXPECT_THAT(ListViewHelper::GetSelectedItems(m_listView.get()), ElementsAre(1));
}

TEST_F(ListViewHelperTest, FocusItem)
{
	ListViewHelper::FocusItem(m_listView.get(), 1, true);
	EXPECT_EQ(ListView_GetItemState(m_listView.get(), 1, LVIS_FOCUSED),
		static_cast<UINT>(LVIS_FOCUSED));

	ListViewHelper::FocusItem(m_listView.get(), 1, false);
	EXPECT_EQ(ListView_GetItemState(m_listView.get(), 1, LVIS_FOCUSED), 0u);
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
