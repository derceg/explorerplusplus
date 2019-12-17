// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkHelper.h"
#include "MainResource.h"
#include "../Helper/Macros.h"
#include <algorithm>

int CALLBACK SortByName(const BookmarkItem *firstItem, const BookmarkItem *secondItem);
int CALLBACK SortByLocation(const BookmarkItem *firstItem, const BookmarkItem *secondItem);
int CALLBACK SortByDateAdded(const BookmarkItem *firstItem, const BookmarkItem *secondItem);
int CALLBACK SortByDateModified(const BookmarkItem *firstItem, const BookmarkItem *secondItem);

bool NBookmarkHelper::IsFolder(const std::unique_ptr<BookmarkItem> &bookmarkItem)
{
	return bookmarkItem->IsFolder();
}

bool NBookmarkHelper::IsBookmark(const std::unique_ptr<BookmarkItem> &bookmarkItem)
{
	return bookmarkItem->IsBookmark();
}

VariantBookmark &NBookmarkHelper::GetBookmarkItem(CBookmarkFolder &ParentBookmarkFolder,
	const std::wstring &guid)
{
	auto itr = std::find_if(ParentBookmarkFolder.begin(),ParentBookmarkFolder.end(),
		[&guid](VariantBookmark &variantBookmark) -> BOOL
		{
			if(variantBookmark.type() == typeid(CBookmarkFolder))
			{
				CBookmarkFolder BookmarkFolder = boost::get<CBookmarkFolder>(variantBookmark);
				return BookmarkFolder.GetGUID() == guid;
			}
			else
			{
				CBookmark Bookmark = boost::get<CBookmark>(variantBookmark);
				return Bookmark.GetGUID() == guid;
			}
		}
	);

	assert(itr != ParentBookmarkFolder.end());

	return *itr;
}

int CALLBACK NBookmarkHelper::Sort(SortMode_t SortMode, const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	if(firstItem->GetType() == BookmarkItem::Type::Folder &&
		secondItem->GetType() == BookmarkItem::Type::Bookmark)
	{
		return -1;
	}
	else if(firstItem->GetType() == BookmarkItem::Type::Bookmark &&
		secondItem->GetType() == BookmarkItem::Type::Folder)
	{
		return 1;
	}
	else
	{
		int iRes = 0;

		switch(SortMode)
		{
		case SM_NAME:
			iRes = SortByName(firstItem,secondItem);
			break;

		case SM_LOCATION:
			iRes = SortByLocation(firstItem,secondItem);
			break;

		case SM_DATE_ADDED:
			iRes = SortByDateAdded(firstItem,secondItem);
			break;

		case SM_DATE_MODIFIED:
			iRes = SortByDateModified(firstItem,secondItem);
			break;

		default:
			assert(false);
			break;
		}

		return iRes;
	}
}

int CALLBACK SortByName(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	return firstItem->GetName().compare(secondItem->GetName());
}

int CALLBACK SortByLocation(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	if(firstItem->GetType() == BookmarkItem::Type::Folder &&
		secondItem->GetType() == BookmarkItem::Type::Folder)
	{
		return 0;
	}
	else if (firstItem->GetType() == BookmarkItem::Type::Folder &&
		secondItem->GetType() == BookmarkItem::Type::Bookmark)
	{
		return -1;
	}
	else if (firstItem->GetType() == BookmarkItem::Type::Bookmark &&
		secondItem->GetType() == BookmarkItem::Type::Folder)
	{
		return 1;
	}
	else
	{
		return firstItem->GetLocation().compare(secondItem->GetLocation());
	}
}

int CALLBACK SortByDateAdded(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	FILETIME firstItemDateCreated = firstItem->GetDateCreated();
	FILETIME secondItemDateCreated = secondItem->GetDateCreated();
	return CompareFileTime(&firstItemDateCreated, &secondItemDateCreated);
}

int CALLBACK SortByDateModified(const BookmarkItem *firstItem,
	const BookmarkItem *secondItem)
{
	FILETIME firstItemDateModified = firstItem->GetDateModified();
	FILETIME secondItemDateModified = secondItem->GetDateModified();
	return CompareFileTime(&firstItemDateModified, &secondItemDateModified);
}