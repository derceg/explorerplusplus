// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkColumnHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "MainResource.h"

namespace
{

std::weak_ordering CompareByName(const BookmarkItem *firstItem, const BookmarkItem *secondItem)
{
	return StrCmpLogicalW(firstItem->GetName().c_str(), secondItem->GetName().c_str()) <=> 0;
}

std::weak_ordering CompareByLocation(const BookmarkItem *firstItem, const BookmarkItem *secondItem)
{
	if (firstItem->IsFolder() && secondItem->IsFolder())
	{
		return CompareByName(firstItem, secondItem);
	}
	else
	{
		return firstItem->GetLocation() <=> secondItem->GetLocation();
	}
}

std::weak_ordering CompareByDateAdded(const BookmarkItem *firstItem, const BookmarkItem *secondItem)
{
	FILETIME firstItemDateCreated = firstItem->GetDateCreated();
	FILETIME secondItemDateCreated = secondItem->GetDateCreated();
	return CompareFileTime(&firstItemDateCreated, &secondItemDateCreated) <=> 0;
}

std::weak_ordering CompareByDateModified(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	FILETIME firstItemDateModified = firstItem->GetDateModified();
	FILETIME secondItemDateModified = secondItem->GetDateModified();
	return CompareFileTime(&firstItemDateModified, &secondItemDateModified) <=> 0;
}

}

UINT GetBookmarkColumnStringId(BookmarkColumn column)
{
	switch (column)
	{
	case BookmarkColumn::Name:
		return IDS_BOOKMARKS_COLUMN_NAME;

	case BookmarkColumn::Location:
		return IDS_BOOKMARKS_COLUMN_LOCATION;

	case BookmarkColumn::DateCreated:
		return IDS_BOOKMARKS_COLUMN_DATE_CREATED;

	case BookmarkColumn::DateModified:
		return IDS_BOOKMARKS_COLUMN_DATE_MODIFIED;
	}

	LOG(FATAL) << "Invalid bookmark column type";
}

std::weak_ordering CompareBookmarksByColumn(BookmarkColumn column, const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	if (firstItem->IsFolder() && secondItem->IsBookmark())
	{
		return std::weak_ordering::less;
	}
	else if (firstItem->IsBookmark() && secondItem->IsFolder())
	{
		return std::weak_ordering::greater;
	}
	else
	{
		switch (column)
		{
		case BookmarkColumn::Name:
			return CompareByName(firstItem, secondItem);

		case BookmarkColumn::Location:
			return CompareByLocation(firstItem, secondItem);

		case BookmarkColumn::DateCreated:
			return CompareByDateAdded(firstItem, secondItem);

		case BookmarkColumn::DateModified:
			return CompareByDateModified(firstItem, secondItem);
		}

		LOG(FATAL) << "Invalid bookmark column type";
	}
}
