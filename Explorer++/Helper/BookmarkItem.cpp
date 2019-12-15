// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkItem.h"
#include "Helper.h"

BookmarkItem::BookmarkItem(std::optional<std::wstring> guid, std::wstring_view name,
	std::optional<std::wstring> location) :
	m_type(location ? Type::Bookmark : Type::Folder),
	m_guid(guid ? *guid : CreateGUID()),
	m_parent(nullptr),
	m_name(name)
{
	GetSystemTimeAsFileTime(&m_dateCreated);
	m_dateModified = m_dateCreated;
}

BookmarkItem::Type BookmarkItem::GetType() const
{
	return m_type;
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
}

FILETIME BookmarkItem::GetDateCreated() const
{
	return m_dateCreated;
}

FILETIME BookmarkItem::GetDateModified() const
{
	return m_dateModified;
}

void BookmarkItem::AddChild(std::unique_ptr<BookmarkItem> bookmarkItem, size_t index)
{
	if (index >= m_children.size())
	{
		index = m_children.size();
	}

	bookmarkItem->m_parent = this;

	m_children.insert(m_children.begin() + index, std::move(bookmarkItem));
}

const std::vector<std::unique_ptr<BookmarkItem>> &BookmarkItem::GetChildren() const
{
	return m_children;
}

void BookmarkItem::UpdateModificationTime()
{
	GetSystemTimeAsFileTime(&m_dateModified);
}