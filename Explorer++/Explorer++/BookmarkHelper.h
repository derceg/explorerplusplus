#pragma once

#include <unordered_set>
#include "../Helper/Bookmark.h"

namespace NBookmarkHelper
{
	struct GuidEq
	{
		bool operator () (const GUID &guid1,const GUID &guid2) const
		{
			return (IsEqualGUID(guid1,guid2) == TRUE);
		}
	};

	struct GuidHash
	{
		size_t operator () (const GUID &guid) const
		{
			return guid.Data1;
		}
	};

	typedef std::unordered_set<GUID,GuidHash,GuidEq> setExpansion_t;
	typedef boost::variant<CBookmarkFolder &,CBookmark &> variantBookmark_t;

	enum SortMode_t
	{
		SM_NAME = 1,
		SM_LOCATION = 2,
		SM_VISIT_DATE = 3,
		SM_VISIT_COUNT = 4,
		SM_ADDED = 5,
		SM_LAST_MODIFIED = 6
	};

	/* These GUID's are statically defined, so that they will
	always be the same across separate instances of the process. */
	static TCHAR *ROOT_GUID = _T("00000000-0000-0000-0000-000000000001");
	static TCHAR *TOOLBAR_GUID = _T("00000000-0000-0000-0000-000000000002");
	static TCHAR *MENU_GUID = _T("00000000-0000-0000-0000-000000000003");

	int CALLBACK		Sort(SortMode_t SortMode,const variantBookmark_t BookmarkItem1,const variantBookmark_t BookmarkItem2);

	variantBookmark_t	GetBookmarkItem(CBookmarkFolder &ParentBookmarkFolder,const GUID &guid);
}