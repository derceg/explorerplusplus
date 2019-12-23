// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkItem.h"
#include "../Helper/Helper.h"

BookmarkItem::BookmarkItem(std::optional<std::wstring> guid, std::wstring_view name,
	std::optional<std::wstring> location) :
	m_type(location ? Type::Bookmark : Type::Folder),
	m_guid(guid ? *guid : CreateGUID()),
	m_parent(nullptr),
	m_name(name),
	m_location(location ? *location : std::wstring())
{
	GetSystemTimeAsFileTime(&m_dateCreated);
	m_dateModified = m_dateCreated;
}

bool BookmarkItem::IsFolder() const
{
	return m_type == Type::Folder;
}

bool BookmarkItem::IsBookmark() const
{
	return m_type == Type::Bookmark;
}

BookmarkItem::Type BookmarkItem::GetType() const
{
	return m_type;
}

BookmarkItem *BookmarkItem::GetParent()
{
	return m_parent;
}

const BookmarkItem *BookmarkItem::GetParent() const
{
	return m_parent;
}

std::wstring BookmarkItem::GetGUID() const
{
	return m_guid;
}

std::wstring BookmarkItem::GetName() const
{
	return m_name;
}

void BookmarkItem::SetName(std::wstring_view name)
{
	m_name = name;

	UpdateModificationTime();

	updatedSignal.m_signal(*this, PropertyType::Name);
}

std::wstring BookmarkItem::GetLocation() const
{
	return m_location;
}

void BookmarkItem::SetLocation(std::wstring_view location)
{
	assert(m_type == Type::Bookmark);

	m_location = location;

	UpdateModificationTime();

	updatedSignal.m_signal(*this, PropertyType::Location);
}

FILETIME BookmarkItem::GetDateCreated() const
{
	return m_dateCreated;
}

void BookmarkItem::SetDateCreated(const FILETIME &dateCreated)
{
	m_dateCreated = dateCreated;

	updatedSignal.m_signal(*this, PropertyType::DateCreated);
}

FILETIME BookmarkItem::GetDateModified() const
{
	return m_dateModified;
}

void BookmarkItem::SetDateModified(const FILETIME &dateModified)
{
	m_dateModified = dateModified;

	updatedSignal.m_signal(*this, PropertyType::DateModified);
}

void BookmarkItem::AddChild(std::unique_ptr<BookmarkItem> bookmarkItem)
{
	AddChild(std::move(bookmarkItem), m_children.size());
}

void BookmarkItem::AddChild(std::unique_ptr<BookmarkItem> bookmarkItem, size_t index)
{
	assert(m_type == Type::Folder);

	if (index > m_children.size())
	{
		index = m_children.size();
	}

	bookmarkItem->m_parent = this;

	m_children.insert(m_children.begin() + index, std::move(bookmarkItem));

	UpdateModificationTime();
}

std::unique_ptr<BookmarkItem> BookmarkItem::RemoveChild(size_t index)
{
	assert(m_type == Type::Folder);

	if (index >= m_children.size())
	{
		return nullptr;
	}

	m_children[index]->m_parent = nullptr;

	auto erasedItem = std::move(m_children[index]);

	m_children.erase(m_children.begin() + index);

	UpdateModificationTime();

	return erasedItem;
}

std::optional<size_t> BookmarkItem::GetChildIndex(const BookmarkItem *bookmarkItem) const
{
	assert(m_type == Type::Folder);

	auto itr = std::find_if(m_children.begin(), m_children.end(), [bookmarkItem] (const auto &item) {
		return item.get() == bookmarkItem;
	});

	if (itr == m_children.end())
	{
		return std::nullopt;
	}

	return itr - m_children.begin();
}

bool BookmarkItem::HasChildFolder() const
{
	assert(m_type == Type::Folder);

	auto itr = std::find_if(m_children.begin(), m_children.end(), [] (const auto &item) {
		return item->IsFolder();
	});

	return (itr != m_children.end());
}

const std::vector<std::unique_ptr<BookmarkItem>> &BookmarkItem::GetChildren() const
{
	assert(m_type == Type::Folder);

	return m_children;
}

void BookmarkItem::UpdateModificationTime()
{
	GetSystemTimeAsFileTime(&m_dateModified);
}