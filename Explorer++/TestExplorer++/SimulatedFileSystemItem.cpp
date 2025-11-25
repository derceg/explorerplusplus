// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedFileSystemItem.h"
#include "PidlTestHelper.h"
#include <boost/algorithm/string/join.hpp>
#include <list>

SimulatedFileSystemItem::SimulatedFileSystemItem(const std::wstring &name, ShellItemType itemType,
	ShellItemExtraAttributes extraAttributes) :
	m_name(name),
	m_itemType(itemType),
	m_extraAttributes(extraAttributes)
{
}

PidlAbsolute SimulatedFileSystemItem::GetPidl() const
{
	std::list<std::wstring> pathSegments;
	const auto *currentItem = this;

	while (currentItem)
	{
		pathSegments.push_front(currentItem->m_name);

		currentItem = currentItem->m_parent;
	}

	std::wstring path = boost::algorithm::join(pathSegments, L"\\");
	return CreateSimplePidlForTest(path, nullptr, m_itemType, m_extraAttributes);
}

SimulatedFileSystemItem *SimulatedFileSystemItem::GetParent()
{
	return m_parent;
}

void SimulatedFileSystemItem::SetName(const std::wstring &name,
	std::optional<ShellItemType> itemType)
{
	m_name = name;

	if (itemType)
	{
		m_itemType = *itemType;

		if (m_itemType == ShellItemType::File)
		{
			m_children.clear();
		}
	}
}

void SimulatedFileSystemItem::SetExtraAttributes(ShellItemExtraAttributes extraAttributes)
{
	m_extraAttributes = extraAttributes;
}

SimulatedFileSystemItem *SimulatedFileSystemItem::AddChild(
	std::unique_ptr<SimulatedFileSystemItem> item)
{
	CHECK(m_itemType == ShellItemType::Folder);

	item->m_parent = this;

	auto *rawItem = item.get();
	m_children.push_back(std::move(item));
	return rawItem;
}

std::unique_ptr<SimulatedFileSystemItem> SimulatedFileSystemItem::RemoveChild(
	SimulatedFileSystemItem *item)
{
	CHECK(m_itemType == ShellItemType::Folder);

	auto itr = std::ranges::find_if(m_children,
		[item](const auto &currentItem) { return currentItem.get() == item; });
	CHECK(itr != m_children.end());

	item->m_parent = nullptr;

	auto ownedItem = std::move(*itr);
	m_children.erase(itr);

	return ownedItem;
}

concurrencpp::generator<const SimulatedFileSystemItem *> SimulatedFileSystemItem::GetChildren()
	const
{
	CHECK(m_itemType == ShellItemType::Folder);

	for (auto &child : m_children)
	{
		co_yield child.get();
	}
}

concurrencpp::generator<SimulatedFileSystemItem *> SimulatedFileSystemItem::GetItemsDepthFirst()
{
	co_yield this;

	for (auto &child : m_children)
	{
		// TODO: This can use std::ranges::elements_of() once C++23 support is available.
		for (auto *item : child->GetItemsDepthFirst())
		{
			co_yield item;
		}
	}
}
