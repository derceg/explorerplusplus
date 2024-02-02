// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkItem.h"
#include <glog/logging.h>

BookmarkItem::BookmarkItem(std::optional<std::wstring> guid, std::wstring_view name,
	std::optional<std::wstring> location) :
	m_type(location ? Type::Bookmark : Type::Folder),
	m_guid(guid ? *guid : CreateGUID()),
	m_name(name),
	m_location(location ? *location : std::wstring())
{
}

// Bookmark deserialization constructor.
BookmarkItem::BookmarkItem(std::wstring_view originalGuid, std::wstring_view name,
	std::wstring location, bool internal) :
	m_originalGuid(originalGuid),
	m_type(Type::Bookmark),
	m_name(name),
	m_location(location)
{
	UNREFERENCED_PARAMETER(internal);
}

// Bookmark folder deserialization constructor.
BookmarkItem::BookmarkItem(std::wstring_view originalGuid, std::wstring_view name,
	BookmarkItems &&children) :
	m_originalGuid(originalGuid),
	m_type(Type::Folder),
	m_name(name),
	m_children(std::move(children))
{
	for (auto &child : m_children)
	{
		child->m_parent = this;
	}
}

FILETIME BookmarkItem::GetCurrentDate()
{
	FILETIME fileTime;
	GetSystemTimeAsFileTime(&fileTime);

	return fileTime;
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

std::optional<std::wstring> BookmarkItem::GetOriginalGUID() const
{
	return m_originalGuid;
}

void BookmarkItem::ClearOriginalGUID()
{
	m_originalGuid.reset();
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

BookmarkItem *BookmarkItem::AddChild(std::unique_ptr<BookmarkItem> bookmarkItem)
{
	return AddChild(std::move(bookmarkItem), m_children.size());
}

BookmarkItem *BookmarkItem::AddChild(std::unique_ptr<BookmarkItem> bookmarkItem, size_t index)
{
	assert(m_type == Type::Folder);
	assert(index <= m_children.size());

	bookmarkItem->m_parent = this;

	BookmarkItem *rawBookmarkItem = bookmarkItem.get();
	m_children.insert(m_children.begin() + index, std::move(bookmarkItem));

	UpdateModificationTime();

	return rawBookmarkItem;
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

size_t BookmarkItem::GetChildIndex(const BookmarkItem *bookmarkItem) const
{
	assert(m_type == Type::Folder);

	auto itr = std::find_if(m_children.begin(), m_children.end(),
		[bookmarkItem](const auto &item)
		{
			return item.get() == bookmarkItem;
		});
	CHECK(itr != m_children.end()) << "BookmarkItem not found";

	return itr - m_children.begin();
}

const std::unique_ptr<BookmarkItem> &BookmarkItem::GetChildOwnedPtr(
	const BookmarkItem *bookmarkItem) const
{
	assert(m_type == Type::Folder);

	auto itr = std::find_if(m_children.begin(), m_children.end(),
		[bookmarkItem](const auto &item)
		{
			return item.get() == bookmarkItem;
		});
	CHECK(itr != m_children.end()) << "BookmarkItem not found";

	return *itr;
}

bool BookmarkItem::HasChildFolder() const
{
	assert(m_type == Type::Folder);

	bool anyChildFolders = std::any_of(m_children.begin(), m_children.end(),
		[](const auto &item)
		{
			return item->IsFolder();
		});

	return anyChildFolders;
}

const BookmarkItems &BookmarkItem::GetChildren() const
{
	assert(m_type == Type::Folder);

	return m_children;
}

void BookmarkItem::UpdateModificationTime()
{
	GetSystemTimeAsFileTime(&m_dateModified);
}

void BookmarkItem::VisitRecursively(std::function<void(BookmarkItem *currentItem)> callback)
{
	callback(this);

	if (!IsFolder())
	{
		return;
	}

	for (auto &child : m_children)
	{
		child->VisitRecursively(callback);
	}
}
