// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkItem.h"
#include "../Helper/Helper.h"
#include "../Helper/StringHelper.h"
#include <glog/logging.h>

std::unique_ptr<BookmarkItem> BookmarkItem::MaybeCreateFromSerializedData(
	const SerializedData &data)
{
	if (data.name.empty() || (data.type == Type::Bookmark && data.location.empty()))
	{
		return nullptr;
	}

	if (data.type == Type::Bookmark)
	{
		auto bookmark = std::make_unique<BookmarkItem>(std::nullopt, utf8StrToWstr(data.name),
			utf8StrToWstr(data.location));
		bookmark->m_originalGuid = utf8StrToWstr(data.guid);
		return bookmark;
	}
	else
	{
		auto bookmarkFolder =
			std::make_unique<BookmarkItem>(std::nullopt, utf8StrToWstr(data.name), std::nullopt);
		bookmarkFolder->m_originalGuid = utf8StrToWstr(data.guid);

		for (const auto &child : data.children)
		{
			auto childItem = MaybeCreateFromSerializedData(child);

			if (childItem)
			{
				bookmarkFolder->AddChild(std::move(childItem));
			}
		}

		return bookmarkFolder;
	}
}

BookmarkItem::SerializedData BookmarkItem::ToSerializedData() const
{
	std::vector<SerializedData> children;

	for (const auto &child : m_children)
	{
		children.push_back(child->ToSerializedData());
	}

	return {
		.guid = wstrToUtf8Str(m_guid),
		.type = m_type,
		.name = wstrToUtf8Str(m_name),
		.location = wstrToUtf8Str(m_location),
		.children = std::move(children),
	};
}

BookmarkItem::BookmarkItem(std::optional<std::wstring> guid, std::wstring_view name,
	std::optional<std::wstring> location) :
	m_type(location ? Type::Bookmark : Type::Folder),
	m_guid(guid ? *guid : CreateGUID()),
	m_name(name),
	m_location(location ? *location : std::wstring())
{
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

const std::wstring &BookmarkItem::GetGUID() const
{
	return m_guid;
}

const std::optional<std::wstring> &BookmarkItem::GetOriginalGUID() const
{
	return m_originalGuid;
}

void BookmarkItem::ClearOriginalGUID()
{
	m_originalGuid.reset();
}

const std::wstring &BookmarkItem::GetName() const
{
	return m_name;
}

void BookmarkItem::SetName(std::wstring_view name)
{
	m_name = name;

	UpdateModificationTime();

	updatedSignal.m_signal(*this, PropertyType::Name);
}

const std::wstring &BookmarkItem::GetLocation() const
{
	return m_location;
}

void BookmarkItem::SetLocation(std::wstring_view location)
{
	DCHECK(IsBookmark());

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
	DCHECK(IsFolder());
	CHECK(index <= m_children.size());

	bookmarkItem->m_parent = this;

	BookmarkItem *rawBookmarkItem = bookmarkItem.get();
	m_children.insert(m_children.begin() + index, std::move(bookmarkItem));

	UpdateModificationTime();

	return rawBookmarkItem;
}

std::unique_ptr<BookmarkItem> BookmarkItem::RemoveChild(size_t index)
{
	DCHECK(IsFolder());

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

BookmarkItem *BookmarkItem::GetChildAtIndex(size_t index)
{
	return const_cast<BookmarkItem *>(std::as_const(*this).GetChildAtIndex(index));
}

const BookmarkItem *BookmarkItem::GetChildAtIndex(size_t index) const
{
	DCHECK(IsFolder());

	CHECK(index < m_children.size());
	return m_children[index].get();
}

size_t BookmarkItem::GetChildIndex(const BookmarkItem *bookmarkItem) const
{
	DCHECK(IsFolder());

	auto itr = std::find_if(m_children.begin(), m_children.end(),
		[bookmarkItem](const auto &item) { return item.get() == bookmarkItem; });
	CHECK(itr != m_children.end()) << "BookmarkItem not found";

	return itr - m_children.begin();
}

bool BookmarkItem::HasChildFolder() const
{
	DCHECK(IsFolder());

	bool anyChildFolders = std::any_of(m_children.begin(), m_children.end(),
		[](const auto &item) { return item->IsFolder(); });

	return anyChildFolders;
}

const BookmarkItems &BookmarkItem::GetChildren() const
{
	DCHECK(IsFolder());

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

WeakPtr<BookmarkItem> BookmarkItem::GetWeakPtr()
{
	return m_weakPtrFactory.GetWeakPtr();
}

WeakPtr<BookmarkItem, const BookmarkItem> BookmarkItem::GetWeakPtr() const
{
	return m_weakPtrFactory.GetWeakPtr();
}
