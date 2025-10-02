// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "GeneratorTestHelper.h"
#include "ListViewItemFake.h"
#include "ListViewModelFake.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

TEST(ListViewModelTest, GetItems)
{
	ListViewModelFake model;
	EXPECT_THAT(GeneratorToVector(model.GetItems()), IsEmpty());

	const auto *item1 = model.AddItem();
	const auto *item2 = model.AddItem();
	EXPECT_THAT(GeneratorToVector(model.GetItems()), ElementsAre(item1, item2));
}

TEST(ListViewModelTest, GetNumItems)
{
	ListViewModelFake model;
	EXPECT_EQ(model.GetNumItems(), 0);

	model.AddItem();
	EXPECT_EQ(model.GetNumItems(), 1);

	auto *item2 = model.AddItem();
	EXPECT_EQ(model.GetNumItems(), 2);

	model.RemoveItem(item2);
	EXPECT_EQ(model.GetNumItems(), 1);

	model.RemoveAllItems();
	EXPECT_EQ(model.GetNumItems(), 0);
}

TEST(ListViewModelTest, GetItemIndex)
{
	ListViewModelFake model;
	const auto *item1 = model.AddItem();
	const auto *item2 = model.AddItem();
	const auto *item3 = model.AddItem();
	EXPECT_EQ(model.GetItemIndex(item1), 0);
	EXPECT_EQ(model.GetItemIndex(item2), 1);
	EXPECT_EQ(model.GetItemIndex(item3), 2);
}

TEST(ListViewModelTest, GetItemAtIndex)
{
	ListViewModelFake model;
	const auto *item1 = model.AddItem();
	const auto *item2 = model.AddItem();
	const auto *item3 = model.AddItem();
	EXPECT_EQ(model.GetItemAtIndex(0), item1);
	EXPECT_EQ(model.GetItemAtIndex(1), item2);
	EXPECT_EQ(model.GetItemAtIndex(2), item3);
}

TEST(ListViewModelTest, HasDefaultSortOrder)
{
	auto model =
		std::make_unique<ListViewModelFake>(ListViewModelFake::SortPolicy::DoesntHaveDefault);
	EXPECT_FALSE(model->HasDefaultSortOrder());

	model = std::make_unique<ListViewModelFake>(ListViewModelFake::SortPolicy::HasDefault);
	EXPECT_TRUE(model->HasDefaultSortOrder());
}

TEST(ListViewModelTest, GetSortColumnId)
{
	ListViewModelFake model;
	model.SetSortDetails(ListViewColumnModelFake::COLUMN_DATA_2, SortDirection::Ascending);
	EXPECT_EQ(model.GetSortColumnId(), ListViewColumnModelFake::COLUMN_DATA_2);

	model.SetSortDetails(ListViewColumnModelFake::COLUMN_NAME, SortDirection::Ascending);
	EXPECT_EQ(model.GetSortColumnId(), ListViewColumnModelFake::COLUMN_NAME);

	model.SetSortDetails(std::nullopt, SortDirection::Ascending);
	EXPECT_EQ(model.GetSortColumnId(), std::nullopt);
}

TEST(ListViewModelTest, GetSortDirection)
{
	ListViewModelFake model;
	model.SetSortDetails(ListViewColumnModelFake::COLUMN_DATA_1, SortDirection::Ascending);
	EXPECT_EQ(model.GetSortDirection(), +SortDirection::Ascending);

	model.SetSortDetails(ListViewColumnModelFake::COLUMN_DATA_1, SortDirection::Descending);
	EXPECT_EQ(model.GetSortDirection(), +SortDirection::Descending);
}

TEST(ListViewModelTest, Sorting)
{
	ListViewModelFake model;
	const auto *itemB = model.AddItem(L"B");
	const auto *itemA = model.AddItem(L"A");
	const auto *itemC = model.AddItem(L"C");

	model.SetSortDetails(ListViewColumnModelFake::COLUMN_NAME, SortDirection::Ascending);
	EXPECT_THAT(GeneratorToVector(model.GetItems()), ElementsAre(itemA, itemB, itemC));

	model.SetSortDetails(ListViewColumnModelFake::COLUMN_NAME, SortDirection::Descending);
	EXPECT_THAT(GeneratorToVector(model.GetItems()), ElementsAre(itemC, itemB, itemA));
}

TEST(ListViewModelTest, InsertInSortedPosition)
{
	ListViewModelFake model;
	const auto *itemA = model.AddItem(L"A");
	const auto *itemF = model.AddItem(L"F");
	const auto *itemM = model.AddItem(L"M");

	model.SetSortDetails(ListViewColumnModelFake::COLUMN_NAME, SortDirection::Ascending);

	const auto *itemC = model.AddItem(L"C");
	EXPECT_THAT(GeneratorToVector(model.GetItems()), ElementsAre(itemA, itemC, itemF, itemM));

	model.SetSortDetails(ListViewColumnModelFake::COLUMN_NAME, SortDirection::Descending);

	const auto *itemH = model.AddItem(L"H");
	EXPECT_THAT(GeneratorToVector(model.GetItems()),
		ElementsAre(itemM, itemH, itemF, itemC, itemA));
}

TEST(ListViewModelTest, UpdateSortedPosition)
{
	ListViewModelFake model;
	auto *itemA = model.AddItem(L"A");
	const auto *itemB = model.AddItem(L"B");
	const auto *itemC = model.AddItem(L"C");

	model.SetSortDetails(ListViewColumnModelFake::COLUMN_NAME, SortDirection::Ascending);

	itemA->SetName(L"Z");
	EXPECT_THAT(GeneratorToVector(model.GetItems()), ElementsAre(itemB, itemC, itemA));
}

TEST(ListViewModelTest, AddedSignal)
{
	ListViewModelFake model;

	MockFunction<void(ListViewItem * item, int index)> callback;
	model.itemAddedSignal.AddObserver(callback.AsStdFunction());

	const ListViewItem *callbackItem = nullptr;
	EXPECT_CALL(callback, Call(_, 0))
		.WillOnce(
			[&callbackItem](ListViewItem *addedItem, int index)
			{
				UNREFERENCED_PARAMETER(index);
				callbackItem = addedItem;
			});
	const auto *item = model.AddItem();
	EXPECT_EQ(callbackItem, item);
}

TEST(ListViewModelTest, UpdatedWithoutMoveSignal)
{
	ListViewModelFake model;
	model.SetSortDetails(ListViewColumnModelFake::COLUMN_NAME, SortDirection::Ascending);
	auto *itemA = model.AddItem(L"A");
	model.AddItem(L"C");

	MockFunction<void(ListViewItem * item)> updateCallback;
	model.itemUpdatedSignal.AddObserver(updateCallback.AsStdFunction());

	MockFunction<void(ListViewItem * item, int newIndex)> moveCallback;
	model.itemMovedSignal.AddObserver(moveCallback.AsStdFunction());

	EXPECT_CALL(updateCallback, Call(itemA));
	EXPECT_CALL(moveCallback, Call(_, _)).Times(0);
	itemA->SetName(L"B");
}

TEST(ListViewModelTest, UpdatedWithMoveSignal)
{
	ListViewModelFake model;
	model.SetSortDetails(ListViewColumnModelFake::COLUMN_NAME, SortDirection::Ascending);
	auto *itemA = model.AddItem(L"A");
	model.AddItem(L"B");
	model.AddItem(L"D");

	MockFunction<void(ListViewItem * item)> updateCallback;
	model.itemUpdatedSignal.AddObserver(updateCallback.AsStdFunction());

	MockFunction<void(ListViewItem * item, int newIndex)> moveCallback;
	model.itemMovedSignal.AddObserver(moveCallback.AsStdFunction());

	EXPECT_CALL(updateCallback, Call(itemA));
	EXPECT_CALL(moveCallback, Call(itemA, 1));
	itemA->SetName(L"C");
}

TEST(ListViewModelTest, RemovedSignal)
{
	ListViewModelFake model;
	auto *item = model.AddItem();

	MockFunction<void(const ListViewItem *item)> callback;
	model.itemRemovedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call(item));
	model.RemoveItem(item);
}

TEST(ListViewModelTest, AllItemsRemovedSignal)
{
	ListViewModelFake model;
	model.AddItem();

	MockFunction<void()> callback;
	model.allItemsRemovedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	model.RemoveAllItems();
}

TEST(ListViewModelTest, SortOrderChangedSignal)
{
	ListViewModelFake model;

	MockFunction<void()> callback;
	model.sortOrderChangedSignal.AddObserver(callback.AsStdFunction());

	MockFunction<void(int)> check;
	{
		InSequence seq;

		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(1));
		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(2));
		EXPECT_CALL(callback, Call());
	}

	model.SetSortDetails(ListViewColumnModelFake::COLUMN_DATA_2, SortDirection::Ascending);
	check.Call(1);
	model.SetSortDetails(ListViewColumnModelFake::COLUMN_DATA_2, SortDirection::Descending);
	check.Call(2);
	model.SetSortDetails(std::nullopt, SortDirection::Ascending);
}
