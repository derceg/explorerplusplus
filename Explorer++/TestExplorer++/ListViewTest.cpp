// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ListView.h"
#include "GeneratorTestHelper.h"
#include "KeyboardStateFake.h"
#include "LabelEditHandler.h"
#include "ListViewColumnModelFake.h"
#include "ListViewItemFake.h"
#include "ListViewModelFake.h"
#include "ResourceLoaderFake.h"
#include "../Helper/Helper.h"
#include "../Helper/WindowHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace testing;

namespace
{

class ListViewDelegateMock : public ListViewDelegate
{
public:
	MOCK_METHOD(bool, OnItemRenamed, (ListViewItem * item, const std::wstring &name), (override));
	MOCK_METHOD(void, OnItemsActivated, (const std::vector<ListViewItem *> &items), (override));
	MOCK_METHOD(void, OnItemsRemoved,
		(const std::vector<ListViewItem *> &items, RemoveMode removeMode), (override));
	MOCK_METHOD(void, OnItemsCopied, (const std::vector<ListViewItem *> &items), (override));
	MOCK_METHOD(void, OnItemsCut, (const std::vector<ListViewItem *> &items), (override));
	MOCK_METHOD(void, OnPaste, (ListViewItem * lastSelectedItemOpt), (override));
	MOCK_METHOD(void, OnShowBackgroundContextMenu, (const POINT &ptScreen), (override));
	MOCK_METHOD(void, OnShowItemContextMenu,
		(const std::vector<ListViewItem *> &items, const POINT &ptScreen), (override));
	MOCK_METHOD(void, OnShowHeaderContextMenu, (const POINT &ptScreen), (override));
	MOCK_METHOD(void, OnBeginDrag, (const std::vector<ListViewItem *> &items), (override));
};

}

class ListViewTest : public Test
{
protected:
	void SetUp() override
	{
		m_parentWindow.reset(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_parentWindow, nullptr);

		// The control needs to have a non-zero width/height so that calls to
		// ListView::MaybeGetItemAtPoint() succeed.
		m_listViewWindow = CreateWindow(WC_LISTVIEW, L"", WS_POPUP | LVS_REPORT, 0, 0, 1000, 1000,
			m_parentWindow.get(), nullptr, GetModuleHandle(nullptr), nullptr);
		ASSERT_NE(m_listViewWindow, nullptr);
	}

	std::unique_ptr<ListView> BuildListView(ListViewModel *model = nullptr)
	{
		auto listView = std::make_unique<ListView>(m_listViewWindow, &m_keyboardState,
			LabelEditHandler::CreateForTest, &m_resourceLoader);
		listView->SetModel(model ? model : &m_model);
		return listView;
	}

	void SimulateClickOnColumnHeader(ListViewColumnId columnId)
	{
		auto index = m_model.GetColumnModel()->MaybeGetColumnVisibleIndex(columnId);
		ASSERT_TRUE(index.has_value());

		HWND header = ListView_GetHeader(m_listViewWindow);
		ASSERT_NE(header, nullptr);

		RECT rect;
		auto res = Header_GetItemRect(header, *index, &rect);
		ASSERT_TRUE(res);

		POINT pt = { rect.left + GetRectWidth(&rect) / 2, rect.top + GetRectHeight(&rect) / 2 };
		SimulateClick(header, pt);
	}

	static void SimulateClick(HWND hwnd, const POINT &ptClient)
	{
		SendMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(ptClient.x, ptClient.y));
		SendMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(ptClient.x, ptClient.y));
	}

	KeyboardStateFake m_keyboardState;
	ResourceLoaderFake m_resourceLoader;

	wil::unique_hwnd m_parentWindow;
	HWND m_listViewWindow = nullptr;
	ListViewModelFake m_model;
};

TEST_F(ListViewTest, InitialColumns)
{
	auto *columnModel = m_model.GetColumnModel();
	columnModel->MoveColumn(ListViewColumnModelFake::COLUMN_NAME, 1);
	columnModel->SetColumnVisible(ListViewColumnModelFake::COLUMN_DATA_2, false);

	auto listView = BuildListView();
	EXPECT_THAT(listView->GetColumnsInVisibleOrderForTesting(),
		ElementsAre(ListViewColumnModelFake::COLUMN_DATA_1, ListViewColumnModelFake::COLUMN_NAME));
}

TEST_F(ListViewTest, ColumnOrdering)
{
	auto listView = BuildListView();

	auto *columnModel = m_model.GetColumnModel();
	columnModel->MoveColumn(ListViewColumnModelFake::COLUMN_NAME, 1);
	EXPECT_EQ(listView->GetColumnsInVisibleOrderForTesting(),
		GeneratorToVector(columnModel->GetVisibleColumnIds()));

	columnModel->MoveColumn(ListViewColumnModelFake::COLUMN_DATA_2, 0);
	EXPECT_EQ(listView->GetColumnsInVisibleOrderForTesting(),
		GeneratorToVector(columnModel->GetVisibleColumnIds()));
}

TEST_F(ListViewTest, ColumnVisibilityChange)
{
	auto listView = BuildListView();

	auto *columnModel = m_model.GetColumnModel();
	m_model.GetColumnModel()->SetColumnVisible(ListViewColumnModelFake::COLUMN_DATA_1, false);
	EXPECT_EQ(listView->GetColumnsInVisibleOrderForTesting(),
		GeneratorToVector(columnModel->GetVisibleColumnIds()));

	m_model.GetColumnModel()->SetColumnVisible(ListViewColumnModelFake::COLUMN_DATA_1, true);
	EXPECT_EQ(listView->GetColumnsInVisibleOrderForTesting(),
		GeneratorToVector(columnModel->GetVisibleColumnIds()));
}

TEST_F(ListViewTest, ColumnClicked)
{
	m_model.SetSortDetails(ListViewColumnModelFake::COLUMN_NAME, SortDirection::Ascending);

	auto listView = BuildListView();

	SimulateClickOnColumnHeader(ListViewColumnModelFake::COLUMN_DATA_1);
	EXPECT_EQ(m_model.GetSortColumnId(), ListViewColumnModelFake::COLUMN_DATA_1);
	EXPECT_EQ(m_model.GetSortDirection(), +SortDirection::Ascending);

	SimulateClickOnColumnHeader(ListViewColumnModelFake::COLUMN_DATA_1);
	EXPECT_EQ(m_model.GetSortColumnId(), ListViewColumnModelFake::COLUMN_DATA_1);
	EXPECT_EQ(m_model.GetSortDirection(), +SortDirection::Descending);
}

TEST_F(ListViewTest, ColumnClickedWithDefaultSortOrder)
{
	ListViewModelFake model(ListViewModel::SortPolicy::HasDefault);
	model.SetSortDetails(ListViewColumnModelFake::COLUMN_DATA_1, SortDirection::Ascending);
	auto listView = BuildListView(&model);

	// There is a default sort order, so clicking a column twice should result in the sort order
	// being reset to the default.
	SimulateClickOnColumnHeader(ListViewColumnModelFake::COLUMN_DATA_1);
	SimulateClickOnColumnHeader(ListViewColumnModelFake::COLUMN_DATA_1);
	EXPECT_EQ(model.GetSortColumnId(), std::nullopt);
	EXPECT_EQ(model.GetSortDirection(), +SortDirection::Ascending);
}

TEST_F(ListViewTest, ColumnClickedWithoutDefaultSortOrder)
{
	ListViewModelFake model(ListViewModel::SortPolicy::DoesntHaveDefault);
	auto listView = BuildListView(&model);

	model.SetSortDetails(ListViewColumnModelFake::COLUMN_DATA_1, SortDirection::Ascending);

	// There is no default sort order, so clicking a column twice should simply invert the sort
	// direction twice.
	SimulateClickOnColumnHeader(ListViewColumnModelFake::COLUMN_DATA_1);
	SimulateClickOnColumnHeader(ListViewColumnModelFake::COLUMN_DATA_1);
	EXPECT_EQ(model.GetSortColumnId(), ListViewColumnModelFake::COLUMN_DATA_1);
	EXPECT_EQ(model.GetSortDirection(), +SortDirection::Ascending);
}

TEST_F(ListViewTest, ColumnResized)
{
	auto listView = BuildListView();

	auto *columnModel = m_model.GetColumnModel();
	int targetIndex = 0;
	const auto &column =
		columnModel->GetColumnById(columnModel->GetColumnIdAtVisibleIndex(targetIndex));

	int updatedWidth = column.width + 10;
	auto res = ListView_SetColumnWidth(m_listViewWindow, targetIndex, updatedWidth);
	ASSERT_TRUE(res);
	EXPECT_EQ(column.width, updatedWidth);
}

TEST_F(ListViewTest, SortArrow)
{
	auto listView = BuildListView();

	m_model.SetSortDetails(ListViewColumnModelFake::COLUMN_DATA_1, SortDirection::Ascending);
	EXPECT_EQ(listView->GetColumnSortArrowDetailsForTesting(),
		ListView::ColumnSortArrowDetails(ListViewColumnModelFake::COLUMN_DATA_1,
			SortDirection::Ascending));

	m_model.SetSortDetails(ListViewColumnModelFake::COLUMN_DATA_2, SortDirection::Descending);
	EXPECT_EQ(listView->GetColumnSortArrowDetailsForTesting(),
		ListView::ColumnSortArrowDetails(ListViewColumnModelFake::COLUMN_DATA_2,
			SortDirection::Descending));

	// Hiding the column used for sorting should result in no sort arrow should be shown.
	m_model.GetColumnModel()->SetColumnVisible(ListViewColumnModelFake::COLUMN_DATA_2, false);
	EXPECT_EQ(listView->GetColumnSortArrowDetailsForTesting(), std::nullopt);

	m_model.SetSortDetails(ListViewColumnModelFake::COLUMN_DATA_2, SortDirection::Ascending);

	// Once the column has been displayed again, the sort arrow should be added.
	m_model.GetColumnModel()->SetColumnVisible(ListViewColumnModelFake::COLUMN_DATA_2, true);
	EXPECT_EQ(listView->GetColumnSortArrowDetailsForTesting(),
		ListView::ColumnSortArrowDetails(ListViewColumnModelFake::COLUMN_DATA_2,
			SortDirection::Ascending));

	m_model.SetSortDetails(std::nullopt, SortDirection::Ascending);
	EXPECT_EQ(listView->GetColumnSortArrowDetailsForTesting(), std::nullopt);
}

TEST_F(ListViewTest, InitialItems)
{
	m_model.AddItem();
	m_model.AddItem();
	m_model.AddItem();

	auto listView = BuildListView();

	ASSERT_EQ(listView->GetItemCountForTesting(), m_model.GetNumItems());

	for (int i = 0; i < m_model.GetNumItems(); i++)
	{
		EXPECT_EQ(listView->GetItemAtIndexForTesting(i), m_model.GetItemAtIndex(i));
	}
}

TEST_F(ListViewTest, GetSelectedItems)
{
	const auto *item1 = m_model.AddItem();
	const auto *item2 = m_model.AddItem();
	const auto *item3 = m_model.AddItem();

	auto listView = BuildListView();
	EXPECT_THAT(listView->GetSelectedItems(), IsEmpty());

	listView->SelectItem(item2);
	EXPECT_THAT(listView->GetSelectedItems(), ElementsAre(item2));

	listView->SelectItem(item3);
	EXPECT_THAT(listView->GetSelectedItems(), ElementsAre(item2, item3));

	listView->SelectItem(item1);
	EXPECT_THAT(listView->GetSelectedItems(), ElementsAre(item1, item2, item3));
}

TEST_F(ListViewTest, IsItemSelected)
{
	const auto *item1 = m_model.AddItem();
	const auto *item2 = m_model.AddItem();
	const auto *item3 = m_model.AddItem();

	auto listView = BuildListView();
	EXPECT_FALSE(listView->IsItemSelected(item1));
	EXPECT_FALSE(listView->IsItemSelected(item2));
	EXPECT_FALSE(listView->IsItemSelected(item3));

	listView->SelectItem(item2);
	EXPECT_FALSE(listView->IsItemSelected(item1));
	EXPECT_TRUE(listView->IsItemSelected(item2));
	EXPECT_FALSE(listView->IsItemSelected(item3));

	listView->SelectItem(item3);
	EXPECT_FALSE(listView->IsItemSelected(item1));
	EXPECT_TRUE(listView->IsItemSelected(item2));
	EXPECT_TRUE(listView->IsItemSelected(item3));

	listView->SelectItem(item1);
	EXPECT_TRUE(listView->IsItemSelected(item1));
	EXPECT_TRUE(listView->IsItemSelected(item2));
	EXPECT_TRUE(listView->IsItemSelected(item3));
}

TEST_F(ListViewTest, SelectAllItems)
{
	const auto *item1 = m_model.AddItem();
	const auto *item2 = m_model.AddItem();
	const auto *item3 = m_model.AddItem();

	auto listView = BuildListView();
	listView->SelectAllItems();
	EXPECT_THAT(listView->GetSelectedItems(), ElementsAre(item1, item2, item3));
}

TEST_F(ListViewTest, DeselectAllItems)
{
	m_model.AddItem();
	m_model.AddItem();
	m_model.AddItem();

	auto listView = BuildListView();
	listView->SelectAllItems();

	listView->DeselectAllItems();
	EXPECT_THAT(listView->GetSelectedItems(), IsEmpty());
}

TEST_F(ListViewTest, ItemPosition)
{
	m_model.AddItem();
	const auto *item2 = m_model.AddItem();
	m_model.AddItem();

	auto listView = BuildListView();

	auto item2Rect = listView->GetItemRect(item2);
	POINT item2Origin = { item2Rect.left, item2Rect.top };
	EXPECT_EQ(listView->MaybeGetItemAtPoint(item2Origin), item2);
	EXPECT_EQ(listView->FindNextItemIndex(item2Origin), 2);
}

TEST_F(ListViewTest, HighlightedItem)
{
	const auto *item = m_model.AddItem();

	auto listView = BuildListView();

	listView->SetItemHighlighted(item, true);
	EXPECT_TRUE(listView->IsItemHighlighted(item));

	listView->SetItemHighlighted(item, false);
	EXPECT_FALSE(listView->IsItemHighlighted(item));
}

TEST_F(ListViewTest, InsertMark)
{
	const auto *item1 = m_model.AddItem();
	m_model.AddItem();

	auto listView = BuildListView();

	// There isn't really any way to verify that the insert mark is being drawn, but the ListView
	// class will check the return values of calls that it makes. So, this at least helps verify
	// that the calls are succeeding.
	listView->ShowInsertMark(item1, InsertMarkPosition::After);
	listView->RemoveInsertMark();
}

class ListViewKeyPressTest : public ListViewTest
{
protected:
	void SetUp() override
	{
		ListViewTest::SetUp();

		m_item1 = m_model.AddItem();
		m_item2 = m_model.AddItem();

		m_listView = BuildListView();
		m_listView->SetDelegate(&m_delegate);
		m_listView->SelectItem(m_item1);
	}

	ListViewDelegateMock m_delegate;
	std::unique_ptr<ListView> m_listView;
	ListViewItem *m_item1 = nullptr;
	ListViewItem *m_item2 = nullptr;
};

TEST_F(ListViewKeyPressTest, Enter)
{
	EXPECT_CALL(m_delegate, OnItemsActivated(ElementsAre(m_item1)));
	SendSimulatedKeyPress(m_listViewWindow, VK_RETURN);
}

TEST_F(ListViewKeyPressTest, SelectAll)
{
	m_keyboardState.SetCtrlDown(true);
	SendSimulatedKeyPress(m_listViewWindow, 'A');
	EXPECT_THAT(m_listView->GetSelectedItems(), ElementsAre(m_item1, m_item2));
}

TEST_F(ListViewKeyPressTest, Copy)
{
	EXPECT_CALL(m_delegate, OnItemsCopied(ElementsAre(m_item1)));
	m_keyboardState.SetCtrlDown(true);
	SendSimulatedKeyPress(m_listViewWindow, 'C');
}

TEST_F(ListViewKeyPressTest, Cut)
{
	EXPECT_CALL(m_delegate, OnItemsCut(ElementsAre(m_item1)));
	m_keyboardState.SetCtrlDown(true);
	SendSimulatedKeyPress(m_listViewWindow, 'X');
}

TEST_F(ListViewKeyPressTest, Paste)
{
	// When OnPaste() is invoked with a particular item, the clipboard items (if any) should be
	// pasted after that item. However, that's only valid when at least one item is selected and the
	// model has a default ordering. Here, an item is selected, but there is no default ordering.
	// So, no item should be passed to OnPaste().
	EXPECT_CALL(m_delegate, OnPaste(nullptr));
	m_keyboardState.SetCtrlDown(true);
	SendSimulatedKeyPress(m_listViewWindow, 'V');
}

TEST_F(ListViewKeyPressTest, Delete)
{
	EXPECT_CALL(m_delegate, OnItemsRemoved(ElementsAre(m_item1), RemoveMode::Standard));
	SendSimulatedKeyPress(m_listViewWindow, VK_DELETE);

	EXPECT_CALL(m_delegate, OnItemsRemoved(ElementsAre(m_item1), RemoveMode::Permanent));
	m_keyboardState.SetShiftDown(true);
	SendSimulatedKeyPress(m_listViewWindow, VK_DELETE);
}

TEST_F(ListViewKeyPressTest, NoSelection)
{
	// Key presses like this should have no effect if no items are selected.
	m_listView->DeselectAllItems();
	EXPECT_CALL(m_delegate, OnItemsRemoved(_, _)).Times(0);
	SendSimulatedKeyPress(m_listViewWindow, VK_DELETE);
}
