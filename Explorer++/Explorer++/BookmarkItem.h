// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SignalWrapper.h"
#include "../Helper/Macros.h"
#include <optional>
#include <vector>

class BookmarkItem
{
public:

	enum class Type
	{
		Folder = 0,
		Bookmark = 1
	};

	enum class PropertyType
	{
		Name,
		Location,
		DateCreated,
		DateModified
	};

	BookmarkItem(std::optional<std::wstring> guid, std::wstring_view name,
		std::optional<std::wstring> location);

	bool IsFolder() const;
	bool IsBookmark() const;
	Type GetType() const;

	BookmarkItem *GetParent();
	const BookmarkItem *GetParent() const;

	std::wstring GetGUID() const;

	std::wstring GetName() const;
	void SetName(std::wstring_view name);

	std::wstring GetLocation() const;
	void SetLocation(std::wstring_view location);

	FILETIME GetDateCreated() const;
	void SetDateCreated(const FILETIME &dateCreated);

	FILETIME GetDateModified() const;
	void SetDateModified(const FILETIME &dateModified);

	void AddChild(std::unique_ptr<BookmarkItem> bookmarkItem);
	void AddChild(std::unique_ptr<BookmarkItem> bookmarkItem, size_t index);
	std::unique_ptr<BookmarkItem> RemoveChild(size_t index);
	bool HasChildFolder() const;

	std::optional<size_t> GetChildIndex(const BookmarkItem *bookmarkItem) const;

	const std::vector<std::unique_ptr<BookmarkItem>> &GetChildren() const;

	// Signals
	SignalWrapper<BookmarkItem, void(BookmarkItem &bookmarkItem, PropertyType propertyType)> updatedSignal;

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