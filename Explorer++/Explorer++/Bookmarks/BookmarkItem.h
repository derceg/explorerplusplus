// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/SignalWrapper.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/core/noncopyable.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

class BookmarkItem;

using BookmarkItems = std::vector<std::unique_ptr<BookmarkItem>>;

// Represents both a bookmark and a bookmark folder. Each folder has the ability to contain other
// bookmark items.
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

	struct SerializedData
	{
		std::string guid;
		Type type;
		std::string name;
		std::string location;
		std::vector<SerializedData> children;
	};

	static std::unique_ptr<BookmarkItem> MaybeCreateFromSerializedData(const SerializedData &data);
	SerializedData ToSerializedData() const;

	BookmarkItem(std::optional<std::wstring> guid, std::wstring_view name,
		std::optional<std::wstring> location);

	bool IsFolder() const;
	bool IsBookmark() const;
	Type GetType() const;

	BookmarkItem *GetParent();
	const BookmarkItem *GetParent() const;

	const std::wstring &GetGUID() const;
	const std::optional<std::wstring> &GetOriginalGUID() const;
	void ClearOriginalGUID();

	const std::wstring &GetName() const;
	void SetName(std::wstring_view name);

	const std::wstring &GetLocation() const;
	void SetLocation(std::wstring_view location);

	FILETIME GetDateCreated() const;
	void SetDateCreated(const FILETIME &dateCreated);

	FILETIME GetDateModified() const;
	void SetDateModified(const FILETIME &dateModified);

	BookmarkItem *AddChild(std::unique_ptr<BookmarkItem> bookmarkItem);
	BookmarkItem *AddChild(std::unique_ptr<BookmarkItem> bookmarkItem, size_t index);
	std::unique_ptr<BookmarkItem> RemoveChild(size_t index);
	bool HasChildFolder() const;

	BookmarkItem *GetChildAtIndex(size_t index);
	const BookmarkItem *GetChildAtIndex(size_t index) const;
	size_t GetChildIndex(const BookmarkItem *bookmarkItem) const;

	const BookmarkItems &GetChildren() const;

	void VisitRecursively(std::function<void(BookmarkItem *currentItem)> callback);

	WeakPtr<BookmarkItem> GetWeakPtr();
	WeakPtr<BookmarkItem, const BookmarkItem> GetWeakPtr() const;

	// Signals
	SignalWrapper<BookmarkItem, void(BookmarkItem &bookmarkItem, PropertyType propertyType)>
		updatedSignal;

private:
	static FILETIME GetCurrentDate();
	void UpdateModificationTime();

	const Type m_type;
	const std::wstring m_guid;

	// If this item was deserialized from a dragged bookmark item or one on the clipboard, this will
	// contain the guid associated with the original item (the one that was dragged or copied).
	std::optional<std::wstring> m_originalGuid;

	BookmarkItem *m_parent = nullptr;
	std::wstring m_name;
	std::wstring m_location;
	FILETIME m_dateCreated = GetCurrentDate();
	FILETIME m_dateModified = m_dateCreated;
	BookmarkItems m_children;

	WeakPtrFactory<BookmarkItem> m_weakPtrFactory{ this };
};

// TODO: NLOHMANN_JSON_SERIALIZE_ENUM_STRICT should be used once available.
NLOHMANN_JSON_SERIALIZE_ENUM(BookmarkItem::Type,
	{ { BookmarkItem::Type::Folder, "folder" }, { BookmarkItem::Type::Bookmark, "bookmark" } })

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BookmarkItem::SerializedData, guid, type, name, location,
	children)
