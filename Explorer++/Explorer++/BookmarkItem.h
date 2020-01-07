// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SignalWrapper.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include <optional>
#include <vector>

class BookmarkItem;

using BookmarkItems = std::vector<std::unique_ptr<BookmarkItem>>;

// Represents both a bookmark and a bookmark folder. Each folder has the ability
// to contain other bookmark items.
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

	friend class cereal::access;

	template <class Archive>
	void serialize(Archive &archive)
	{
		archive(m_type, m_name, m_location, m_children);
	}

	template <class Archive>
	static void load_and_construct(Archive &archive, cereal::construct<BookmarkItem> &construct)
	{
		Type type;
		std::wstring name;
		std::wstring location;
		BookmarkItems children;

		archive(type, name, location, children);

		if (type == Type::Bookmark)
		{
			construct(name, location);
		}
		else
		{
			construct(name, std::move(children));
		}
	}

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

	const BookmarkItems &GetChildren() const;

	// Signals
	SignalWrapper<BookmarkItem, void(BookmarkItem &bookmarkItem, PropertyType propertyType)> updatedSignal;

private:

	DISALLOW_COPY_AND_ASSIGN(BookmarkItem);

	// Used exclusively when deserializing. The advantage here mainly comes from
	// the second of these methods. It allows the list of children to be
	// directly imported. Without that method, you would have to call AddChild()
	// for each child item and there's no need to do that when you already have
	// a list of the children in the correct format.
	BookmarkItem(std::wstring_view name, std::wstring location);
	BookmarkItem(std::wstring_view name, BookmarkItems &&children);

	static FILETIME GetCurrentDate();

	void UpdateModificationTime();

	const Type m_type;
	std::wstring m_guid = CreateGUID();

	BookmarkItem *m_parent = nullptr;

	std::wstring m_name;

	std::wstring m_location;

	FILETIME m_dateCreated = GetCurrentDate();
	FILETIME m_dateModified = m_dateCreated;

	BookmarkItems m_children;
};