// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/MovableModel.h"
#include <boost/signals2.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class ItemFake
{
public:
	using UpdatedSignal = boost::signals2::signal<void(ItemFake *item)>;

	ItemFake(const std::wstring &name) : m_name(name)
	{
	}

	std::wstring GetName() const
	{
		return m_name;
	}

	void SetName(const std::wstring &name)
	{
		if (name == m_name)
		{
			return;
		}

		m_name = name;

		m_updatedSignal(this);
	}

	boost::signals2::connection AddUpdatedObserver(const UpdatedSignal::slot_type &observer)
	{
		return m_updatedSignal.connect(observer);
	}

private:
	std::wstring m_name;

	UpdatedSignal m_updatedSignal;
};

class ItemFakeModel : public MovableModel<ItemFake>
{
};

class ItemFakeModelObserverMock
{
public:
	ItemFakeModelObserverMock(ItemFakeModel *model)
	{
		m_connections.push_back(model->AddItemAddedObserver(
			std::bind_front(&ItemFakeModelObserverMock::OnItemAdded, this)));
		m_connections.push_back(model->AddItemUpdatedObserver(
			std::bind_front(&ItemFakeModelObserverMock::OnItemUpdated, this)));
		m_connections.push_back(model->AddItemMovedObserver(
			std::bind_front(&ItemFakeModelObserverMock::OnItemMoved, this)));
		m_connections.push_back(model->AddItemRemovedObserver(
			std::bind_front(&ItemFakeModelObserverMock::OnItemRemoved, this)));
		m_connections.push_back(model->AddAllItemsRemovedObserver(
			std::bind_front(&ItemFakeModelObserverMock::OnAllItemsRemoved, this)));
	}

	MOCK_METHOD(void, OnItemAdded, (ItemFake * item, size_t index));
	MOCK_METHOD(void, OnItemUpdated, (ItemFake * item));
	MOCK_METHOD(void, OnItemMoved, (ItemFake * item, size_t oldIndex, size_t newIndex));
	MOCK_METHOD(void, OnItemRemoved, (const ItemFake *item, size_t oldIndex));
	MOCK_METHOD(void, OnAllItemsRemoved, ());

private:
	std::vector<boost::signals2::scoped_connection> m_connections;
};

class MovableModelTest : public Test
{
protected:
	MovableModelTest() : m_observer(&m_model)
	{
	}

	ItemFakeModel m_model;
	ItemFakeModelObserverMock m_observer;
};

TEST_F(MovableModelTest, AddItem)
{
	auto item = std::make_unique<ItemFake>(L"Item 1");
	auto rawItem = item.get();

	EXPECT_CALL(m_observer, OnItemAdded(rawItem, 0));
	m_model.AddItem(std::move(item));

	EXPECT_EQ(m_model.GetItems().size(), 1U);
	EXPECT_EQ(m_model.GetItemAtIndex(0), rawItem);
	EXPECT_EQ(m_model.GetItemIndex(rawItem), 0U);

	auto item2 = std::make_unique<ItemFake>(L"Item 2");
	auto rawItem2 = item2.get();

	EXPECT_CALL(m_observer, OnItemAdded(rawItem2, 1));
	m_model.AddItem(std::move(item2));

	EXPECT_EQ(m_model.GetItems().size(), 2U);
	EXPECT_EQ(m_model.GetItemAtIndex(1), rawItem2);
	EXPECT_EQ(m_model.GetItemIndex(rawItem2), 1U);
}

TEST_F(MovableModelTest, AddItemAtIndex)
{
	auto item1 = std::make_unique<ItemFake>(L"Item 1");
	auto *rawItem1 = m_model.AddItem(std::move(item1));

	auto item2 = std::make_unique<ItemFake>(L"Item 2");
	auto *rawItem2 = m_model.AddItem(std::move(item2), 0);

	ASSERT_EQ(m_model.GetItems().size(), 2U);

	EXPECT_EQ(m_model.GetItemAtIndex(0), rawItem2);
	EXPECT_EQ(m_model.GetItemAtIndex(1), rawItem1);

	auto item3 = std::make_unique<ItemFake>(L"Item 3");
	auto *rawItem3 = m_model.AddItem(std::move(item3), 1);

	ASSERT_EQ(m_model.GetItems().size(), 3U);

	EXPECT_EQ(m_model.GetItemAtIndex(0), rawItem2);
	EXPECT_EQ(m_model.GetItemAtIndex(1), rawItem3);
	EXPECT_EQ(m_model.GetItemAtIndex(2), rawItem1);

	auto item4 = std::make_unique<ItemFake>(L"Item 4");
	auto *rawItem4 = m_model.AddItem(std::move(item4), 3);

	ASSERT_EQ(m_model.GetItems().size(), 4U);

	EXPECT_EQ(m_model.GetItemAtIndex(0), rawItem2);
	EXPECT_EQ(m_model.GetItemAtIndex(1), rawItem3);
	EXPECT_EQ(m_model.GetItemAtIndex(2), rawItem1);
	EXPECT_EQ(m_model.GetItemAtIndex(3), rawItem4);

	// Attempting to add an item past the end of the model should result in the item being added to
	// the end.
	auto item5 = std::make_unique<ItemFake>(L"Item 5");
	auto *rawItem5 = m_model.AddItem(std::move(item5), 100);

	ASSERT_EQ(m_model.GetItems().size(), 5U);

	EXPECT_EQ(m_model.GetItemAtIndex(0), rawItem2);
	EXPECT_EQ(m_model.GetItemAtIndex(1), rawItem3);
	EXPECT_EQ(m_model.GetItemAtIndex(2), rawItem1);
	EXPECT_EQ(m_model.GetItemAtIndex(3), rawItem4);
	EXPECT_EQ(m_model.GetItemAtIndex(4), rawItem5);
}

TEST_F(MovableModelTest, UpdateItem)
{
	auto item = std::make_unique<ItemFake>(L"Item");
	auto rawItem = m_model.AddItem(std::move(item));

	EXPECT_CALL(m_observer, OnItemUpdated(rawItem));
	rawItem->SetName(L"Updated name");
}

TEST_F(MovableModelTest, MoveItem)
{
	auto item1 = std::make_unique<ItemFake>(L"Item 1");
	auto *rawItem1 = m_model.AddItem(std::move(item1));

	auto item2 = std::make_unique<ItemFake>(L"Item 2");
	auto *rawItem2 = m_model.AddItem(std::move(item2));

	auto item3 = std::make_unique<ItemFake>(L"Item 3");
	auto *rawItem3 = m_model.AddItem(std::move(item3));

	auto item4 = std::make_unique<ItemFake>(L"Item 4");
	auto *rawItem4 = m_model.AddItem(std::move(item4));

	EXPECT_CALL(m_observer, OnItemMoved(rawItem2, 1, 3));
	m_model.MoveItem(rawItem2, 3);

	// The number of items shouldn't have changed.
	ASSERT_EQ(m_model.GetItems().size(), 4U);

	EXPECT_EQ(m_model.GetItemAtIndex(0), rawItem1);
	EXPECT_EQ(m_model.GetItemAtIndex(1), rawItem3);
	EXPECT_EQ(m_model.GetItemAtIndex(2), rawItem4);
	EXPECT_EQ(m_model.GetItemAtIndex(3), rawItem2);

	EXPECT_CALL(m_observer, OnItemMoved(rawItem4, 2, 0));
	m_model.MoveItem(rawItem4, 0);

	ASSERT_EQ(m_model.GetItems().size(), 4U);

	EXPECT_EQ(m_model.GetItemAtIndex(0), rawItem4);
	EXPECT_EQ(m_model.GetItemAtIndex(1), rawItem1);
	EXPECT_EQ(m_model.GetItemAtIndex(2), rawItem3);
	EXPECT_EQ(m_model.GetItemAtIndex(3), rawItem2);

	// Attempting to move an item past the end of the model should result in the item being moved to
	// the end.
	EXPECT_CALL(m_observer, OnItemMoved(rawItem1, 1, 3));
	m_model.MoveItem(rawItem1, 4);

	ASSERT_EQ(m_model.GetItems().size(), 4U);

	EXPECT_EQ(m_model.GetItemAtIndex(0), rawItem4);
	EXPECT_EQ(m_model.GetItemAtIndex(1), rawItem3);
	EXPECT_EQ(m_model.GetItemAtIndex(2), rawItem2);
	EXPECT_EQ(m_model.GetItemAtIndex(3), rawItem1);

	EXPECT_CALL(m_observer, OnItemMoved(rawItem3, 1, 3));
	m_model.MoveItem(rawItem3, 100);

	ASSERT_EQ(m_model.GetItems().size(), 4U);

	EXPECT_EQ(m_model.GetItemAtIndex(0), rawItem4);
	EXPECT_EQ(m_model.GetItemAtIndex(1), rawItem2);
	EXPECT_EQ(m_model.GetItemAtIndex(2), rawItem1);
	EXPECT_EQ(m_model.GetItemAtIndex(3), rawItem3);
}

TEST_F(MovableModelTest, RemoveItem)
{
	auto item = std::make_unique<ItemFake>(L"Item");
	auto rawItem = m_model.AddItem(std::move(item));

	EXPECT_CALL(m_observer, OnItemRemoved(rawItem, 0));
	m_model.RemoveItem(rawItem);

	EXPECT_EQ(m_model.GetItems().size(), 0U);
}

TEST_F(MovableModelTest, RemoveAllItems)
{
	auto item = std::make_unique<ItemFake>(L"Item");
	m_model.AddItem(std::move(item));

	EXPECT_CALL(m_observer, OnAllItemsRemoved());
	m_model.RemoveAllItems();

	EXPECT_EQ(m_model.GetItems().size(), 0U);
}

TEST_F(MovableModelTest, GetItems)
{
	auto item1 = std::make_unique<ItemFake>(L"Item 1");
	auto *rawItem1 = m_model.AddItem(std::move(item1));

	auto item2 = std::make_unique<ItemFake>(L"Item 2");
	auto *rawItem2 = m_model.AddItem(std::move(item2));

	ASSERT_EQ(m_model.GetItems().size(), 2U);

	auto &storedItem1 = m_model.GetItems().at(0);
	EXPECT_EQ(storedItem1.get(), rawItem1);

	auto &storedItem2 = m_model.GetItems().at(1);
	EXPECT_EQ(storedItem2.get(), rawItem2);
}
