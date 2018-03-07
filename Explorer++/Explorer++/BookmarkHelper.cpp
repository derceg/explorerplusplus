/******************************************************************
 *
 * Project: Explorer++
 * File: BookmarkHelper.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Provides several helper functions for bookmarks.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <stack>
#include <algorithm>
#include "Explorer++_internal.h"
#include "MainImages.h"
#include "BookmarkHelper.h"
#include "MainResource.h"
#include "../Helper/Macros.h"


int CALLBACK SortByName(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);
int CALLBACK SortByLocation(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);
int CALLBACK SortByVisitDate(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);
int CALLBACK SortByVisitCount(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);
int CALLBACK SortByAdded(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);
int CALLBACK SortByLastModified(const NBookmarkHelper::variantBookmark_t BookmarkItem1,const NBookmarkHelper::variantBookmark_t BookmarkItem2);

NBookmarkHelper::variantBookmark_t NBookmarkHelper::GetBookmarkItem(CBookmarkFolder &ParentBookmarkFolder,
	const GUID &guid)
{
	auto itr = std::find_if(ParentBookmarkFolder.begin(),ParentBookmarkFolder.end(),
		[guid](boost::variant<CBookmarkFolder,CBookmark> &variantBookmark) -> BOOL
		{
			if(variantBookmark.type() == typeid(CBookmarkFolder))
			{
				CBookmarkFolder BookmarkFolder = boost::get<CBookmarkFolder>(variantBookmark);
				return IsEqualGUID(BookmarkFolder.GetGUID(),guid);
			}
			else
			{
				CBookmark Bookmark = boost::get<CBookmark>(variantBookmark);
				return IsEqualGUID(Bookmark.GetGUID(),guid);
			}
		}
	);

	assert(itr != ParentBookmarkFolder.end());

	if(itr->type() == typeid(CBookmarkFolder))
	{
		CBookmarkFolder &BookmarkFolder = boost::get<CBookmarkFolder>(*itr);
		return BookmarkFolder;
	}
	else
	{
		CBookmark &Bookmark = boost::get<CBookmark>(*itr);
		return Bookmark;
	}
}

int CALLBACK NBookmarkHelper::Sort(SortMode_t SortMode,const variantBookmark_t BookmarkItem1,
	const variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmark))
	{
		return -1;
	}
	else if(BookmarkItem1.type() == typeid(CBookmark) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		return 1;
	}
	else
	{
		int iRes = 0;

		switch(SortMode)
		{
		case SM_NAME:
			iRes = SortByName(BookmarkItem1,BookmarkItem2);
			break;

		case SM_LOCATION:
			iRes = SortByLocation(BookmarkItem1,BookmarkItem2);
			break;

		case SM_VISIT_DATE:
			iRes = SortByVisitDate(BookmarkItem1,BookmarkItem2);
			break;

		case SM_VISIT_COUNT:
			iRes = SortByVisitCount(BookmarkItem1,BookmarkItem2);
			break;

		case SM_ADDED:
			iRes = SortByAdded(BookmarkItem1,BookmarkItem2);
			break;

		case SM_LAST_MODIFIED:
			iRes = SortByLastModified(BookmarkItem1,BookmarkItem2);
			break;

		default:
			assert(false);
			break;
		}

		return iRes;
	}
}

int CALLBACK SortByName(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		const CBookmarkFolder &BookmarkFolder1 = boost::get<CBookmarkFolder>(BookmarkItem1);
		const CBookmarkFolder &BookmarkFolder2 = boost::get<CBookmarkFolder>(BookmarkItem2);

		return BookmarkFolder1.GetName().compare(BookmarkFolder2.GetName());
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		return Bookmark1.GetName().compare(Bookmark2.GetName());
	}
}

int CALLBACK SortByLocation(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		return 0;
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		return Bookmark1.GetLocation().compare(Bookmark2.GetLocation());
	}
}

int CALLBACK SortByVisitDate(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		return 0;
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		FILETIME ft1 = Bookmark1.GetDateLastVisited();
		FILETIME ft2 = Bookmark2.GetDateLastVisited();

		return CompareFileTime(&ft1,&ft2);
	}
}

int CALLBACK SortByVisitCount(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		return 0;
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		return Bookmark1.GetVisitCount() - Bookmark2.GetVisitCount();
	}
}

int CALLBACK SortByAdded(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		const CBookmarkFolder &BookmarkFolder1 = boost::get<CBookmarkFolder>(BookmarkItem1);
		const CBookmarkFolder &BookmarkFolder2 = boost::get<CBookmarkFolder>(BookmarkItem2);

		FILETIME ft1 = BookmarkFolder1.GetDateCreated();
		FILETIME ft2 = BookmarkFolder2.GetDateCreated();

		return CompareFileTime(&ft1,&ft2);
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		FILETIME ft1 = Bookmark1.GetDateCreated();
		FILETIME ft2 = Bookmark2.GetDateCreated();

		return CompareFileTime(&ft1,&ft2);
	}
}

int CALLBACK SortByLastModified(const NBookmarkHelper::variantBookmark_t BookmarkItem1,
	const NBookmarkHelper::variantBookmark_t BookmarkItem2)
{
	if(BookmarkItem1.type() == typeid(CBookmarkFolder) &&
		BookmarkItem2.type() == typeid(CBookmarkFolder))
	{
		const CBookmarkFolder &BookmarkFolder1 = boost::get<CBookmarkFolder>(BookmarkItem1);
		const CBookmarkFolder &BookmarkFolder2 = boost::get<CBookmarkFolder>(BookmarkItem2);

		FILETIME ft1 = BookmarkFolder1.GetDateModified();
		FILETIME ft2 = BookmarkFolder2.GetDateModified();

		return CompareFileTime(&ft1,&ft2);
	}
	else
	{
		const CBookmark &Bookmark1 = boost::get<CBookmark>(BookmarkItem1);
		const CBookmark &Bookmark2 = boost::get<CBookmark>(BookmarkItem1);

		FILETIME ft1 = Bookmark1.GetDateModified();
		FILETIME ft2 = Bookmark2.GetDateModified();

		return CompareFileTime(&ft1,&ft2);
	}
}