// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <memory>
#include <vector>

// clang-format off
template <class T>
concept UpdatableItem =
	requires(T item, typename boost::signals2::signal<void(T *updatedItem)>::slot_type observer)
	{
		{ item.AddUpdatedObserver(observer) } -> std::same_as<boost::signals2::connection>;
	};
// clang-format on

// A templated model in which an ordered set of items is stored. Items can be:
//
// - Added
// - Updated
// - Moved
// - Removed
//
// Items should be individually updatable. When an item is updated, the model will emit a signal.
// This allows observers to be notified when any item in the model changes, without having to
// subscribe to a signal for each item.
template <UpdatableItem ItemType>
class MovableModel
{
public:
	using ItemsList = std::vector<std::unique_ptr<ItemType>>;

	using ItemAddedSignal = boost::signals2::signal<void(ItemType *item, size_t index)>;
	using ItemUpdatedSignal = boost::signals2::signal<void(ItemType *item)>;
	using ItemMovedSignal =
		boost::signals2::signal<void(ItemType *item, size_t oldIndex, size_t newIndex)>;
	using ItemRemovedSignal = boost::signals2::signal<void(const ItemType *item, size_t oldIndex)>;
	using AllItemsRemovedSignal = boost::signals2::signal<void()>;

	MovableModel() = default;
	virtual ~MovableModel() = default;

	MovableModel(const MovableModel &) = delete;
	MovableModel(MovableModel &&) = delete;
	MovableModel &operator=(const MovableModel &) = delete;
	MovableModel &operator=(MovableModel &&) = delete;

	ItemType *AddItem(std::unique_ptr<ItemType> item)
	{
		return AddItem(std::move(item), m_items.size());
	}

	ItemType *AddItem(std::unique_ptr<ItemType> item, size_t index)
	{
		if (index > m_items.size())
		{
			index = m_items.size();
		}

		item->AddUpdatedObserver(std::bind_front(&MovableModel::OnItemUpdated, this));

		auto *rawItem = item.get();
		m_items.insert(m_items.begin() + index, std::move(item));
		m_itemAddedSignal(rawItem, index);

		return rawItem;
	}

	void MoveItem(ItemType *item, size_t index)
	{
		if (index >= m_items.size())
		{
			index = m_items.size() - 1;
		}

		auto oldIndex = GetItemIndex(item);

		if (index > oldIndex)
		{
			std::rotate(m_items.begin() + oldIndex, m_items.begin() + oldIndex + 1,
				m_items.begin() + index + 1);
		}
		else
		{
			std::rotate(m_items.rend() - oldIndex - 1, m_items.rend() - oldIndex,
				m_items.rend() - index);
		}

		m_itemMovedSignal(item, oldIndex, index);
	}

	void RemoveItem(const ItemType *item)
	{
		auto itr = std::find_if(m_items.begin(), m_items.end(),
			[item](const auto &currentEntry) { return currentEntry.get() == item; });

		if (itr == m_items.end())
		{
			return;
		}

		std::unique_ptr<ItemType> ownedItem = std::move(*itr);
		size_t index = itr - m_items.begin();

		m_items.erase(itr);

		m_itemRemovedSignal(ownedItem.get(), index);
	}

	void RemoveAllItems()
	{
		m_items.clear();

		m_allItemsRemovedSignal();
	}

	const ItemsList &GetItems() const
	{
		return m_items;
	}

	size_t GetItemIndex(const ItemType *item) const
	{
		auto itr = std::find_if(m_items.begin(), m_items.end(),
			[item](const auto &currentEntry) { return currentEntry.get() == item; });
		CHECK(itr != m_items.end()) << "Item not found";

		return itr - m_items.begin();
	}

	ItemType *GetItemAtIndex(size_t index) const
	{
		CHECK_LT(index, m_items.size());
		return m_items[index].get();
	}

	[[nodiscard]] boost::signals2::connection AddItemAddedObserver(
		const typename ItemAddedSignal::slot_type &observer)
	{
		return m_itemAddedSignal.connect(observer);
	}

	[[nodiscard]] boost::signals2::connection AddItemUpdatedObserver(
		const typename ItemUpdatedSignal::slot_type &observer)
	{
		return m_itemUpdatedSignal.connect(observer);
	}

	[[nodiscard]] boost::signals2::connection AddItemMovedObserver(
		const typename ItemMovedSignal::slot_type &observer)
	{
		return m_itemMovedSignal.connect(observer);
	}

	[[nodiscard]] boost::signals2::connection AddItemRemovedObserver(
		const typename ItemRemovedSignal::slot_type &observer)
	{
		return m_itemRemovedSignal.connect(observer);
	}

	[[nodiscard]] boost::signals2::connection AddAllItemsRemovedObserver(
		const typename AllItemsRemovedSignal::slot_type &observer)
	{
		return m_allItemsRemovedSignal.connect(observer);
	}

private:
	void OnItemUpdated(ItemType *item)
	{
		m_itemUpdatedSignal(item);
	}

	ItemsList m_items;

	ItemAddedSignal m_itemAddedSignal;
	ItemUpdatedSignal m_itemUpdatedSignal;
	ItemMovedSignal m_itemMovedSignal;
	ItemRemovedSignal m_itemRemovedSignal;
	AllItemsRemovedSignal m_allItemsRemovedSignal;
};
