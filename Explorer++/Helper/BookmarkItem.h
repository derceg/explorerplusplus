// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Macros.h"
#include <optional>
#include <vector>

class BookmarkItem
{
public:

	enum class Type
	{
		Folder,
		Bookmark
	};

	BookmarkItem(std::optional<std::wstring> guid, std::wstring_view name,
		std::optional<std::wstring> location);

	Type GetType() const;
	std::wstring GetGUID() const;

	std::wstring GetName() const;
	void SetName(std::wstring_view name);

	std::wstring GetLocation() const;
	void SetLocation(std::wstring_view location);

	FILETIME GetDateCreated() const;
	FILETIME GetDateModified() const;

	void AddChild(std::unique_ptr<BookmarkItem> bookmarkItem);
	void AddChild(std::unique_ptr<BookmarkItem> bookmarkItem, size_t index);
	void RemoveChild(size_t index);

	std::optional<size_t> GetChildIndex(const BookmarkItem *bookmarkItem) const;

	const std::vector<std::unique_ptr<BookmarkItem>> &GetChildren() const;

private:

	DISALLOW_COPY_AND_ASSIGN(BookmarkItem);

	void UpdateModificationTime();

	const Type m_type;
	std::wstring m_guid;

	BookmarkItem *m_parent;

	std::wstring m_name;

	std::wstring m_location;

	FILETIME m_dateCreated;
	FILETIME m_dateModified;

	std::vector<std::unique_ptr<BookmarkItem>> m_children;
};