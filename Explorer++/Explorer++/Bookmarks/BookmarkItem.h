// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "SignalWrapper.h"
#include "../Helper/Helper.h"
#include <boost/core/noncopyable.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <optional>
#include <vector>

class BookmarkItem;

using BookmarkItems = std::vector<std::unique_ptr<BookmarkItem>>;

// Represents both a bookmark and a bookmark folder. Each folder has the ability
// to contain other bookmark items.
class BookmarkItem : private boost::noncopyable
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
		archive(m_guid, m_type, m_name, m_location, m_children);
	}

	template <class Archive>
	static void load_and_construct(Archive &archive, cereal::construct<BookmarkItem> &construct)
	{
		std::wstring originalGuid;
		Type type;
		std::wstring name;
		std::wstring location;
		BookmarkItems children;

		archive(originalGuid, type, name, location, children);

		if (type == Type::Bookmark)
		{
			construct(originalGuid, name, location, true);
		}
		else
		{
			construct(originalGuid, name, std::move(children));
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
	std::optional<std::wstring> GetOriginalGUID() const;
	void ClearOriginalGUID();

	std::wstring GetName() const;
	void SetName(std::wstring_view name);

	std::wstring GetLocation() const;
	void SetLocation(std::wstring_view location);

	FILETIME GetDateCreated() const;
	void SetDateCreated(const FILETIME &dateCreated);

	FILETIME GetDateModified() const;
	void SetDateModified(const FILETIME &dateModified);

	BookmarkItem *AddChild(std::unique_ptr<BookmarkItem> bookmarkItem);
	BookmarkItem *AddChild(std::unique_ptr<BookmarkItem> bookmarkItem, size_t index);
	std::unique_ptr<BookmarkItem> RemoveChild(size_t index);
	bool HasChildFolder() const;

	size_t GetChildIndex(const BookmarkItem *bookmarkItem) const;
	const std::unique_ptr<BookmarkItem> &GetChildOwnedPtr(const BookmarkItem *bookmarkItem) const;

	const BookmarkItems &GetChildren() const;

	void VisitRecursively(std::function<void(BookmarkItem *currentItem)> callback);

	// Signals
	SignalWrapper<BookmarkItem, void(BookmarkItem &bookmarkItem, PropertyType propertyType)>
		updatedSignal;

private:
	// Used exclusively when deserializing. The advantage here mainly comes from
	// the second of these methods. It allows the list of children to be
	// directly imported. Without that method, you would have to call AddChild()
	// for each child item and there's no need to do that when you already have
	// a list of the children in the correct format.
	// Also note that the first method has a dummy parameter added to the end.
	// This is because the method would be ambiguous otherwise (it would clash with
	// the public constructor).
	BookmarkItem(std::wstring_view originalGuid, std::wstring_view name, std::wstring location,
		bool internal);
	BookmarkItem(std::wstring_view originalGuid, std::wstring_view name, BookmarkItems &&children);

	static FILETIME GetCurrentDate();

	void UpdateModificationTime();

	const Type m_type;
	std::wstring m_guid = CreateGUID();

	// If this item was deserialized from a dragged bookmark item or one on the
	// clipboard, this will contain the guid associated with the original item
	// (the one that was dragged or copied).
	std::optional<std::wstring> m_originalGuid;

	BookmarkItem *m_parent = nullptr;

	std::wstring m_name;

	std::wstring m_location;

	FILETIME m_dateCreated = GetCurrentDate();
	FILETIME m_dateModified = m_dateCreated;

	BookmarkItems m_children;
};
