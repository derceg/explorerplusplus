/******************************************************************
 *
 * Project: Helper
 * File: Bookmark.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Implements a general bookmarking system.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include <algorithm>
#include "Bookmark.h"
#include "RegistrySettings.h"


#define ROOT_NAME	_T("Bookmarks")

CBookmark::CBookmark(void)
{
	/* Fill out information for the root. */
	StringCchCopy(m_Root.szItemName,SIZEOF_ARRAY(m_Root.szItemName),ROOT_NAME);
	m_Root.Type			= BOOKMARK_TYPE_FOLDER;
	m_Root.NextSibling	= NULL;
	m_Root.FirstChild	= NULL;
}

CBookmark::~CBookmark(void)
{
}

void CBookmark::GetRoot(Bookmark_t *pRoot)
{
	StringCchCopy(pRoot->szItemName,SIZEOF_ARRAY(pRoot->szItemName),
		m_Root.szItemName);
	StringCchCopy(pRoot->szItemDescription,
		SIZEOF_ARRAY(pRoot->szItemDescription),
		m_Root.szItemDescription);

	pRoot->Type				= m_Root.Type;
	pRoot->bShowOnToolbar	= m_Root.bShowOnToolbar;
	pRoot->pHandle			= (void *)&m_Root;
}

void CBookmark::ImportBookmarkInternal(BookmarkInternal_t *pbi,Bookmark_t *pBookmark)
{
	StringCchCopy(pbi->szItemName,SIZEOF_ARRAY(pbi->szItemName),
		pBookmark->szItemName);
	StringCchCopy(pbi->szItemDescription,
		SIZEOF_ARRAY(pbi->szItemDescription),
		pBookmark->szItemDescription);

	pbi->Type			= pBookmark->Type;
	pbi->bShowOnToolbar	= pBookmark->bShowOnToolbar;
	pbi->FirstChild		= NULL;
	pbi->NextSibling	= NULL;

	if(pBookmark->Type == BOOKMARK_TYPE_BOOKMARK)
	{
		/* If this is a bookmark, also copy
		across its location. The calling code
		will be responsible for freeing this. */
		StringCchCopy(pbi->szLocation,SIZEOF_ARRAY(pbi->szLocation),
			pBookmark->szLocation);
	}

	pBookmark->pHandle	= (void *)pbi;
}

void CBookmark::ExportBookmarkInternal(BookmarkInternal_t *pbi,Bookmark_t *pBookmark)
{
	StringCchCopy(pBookmark->szItemName,SIZEOF_ARRAY(pBookmark->szItemName),
		pbi->szItemName);
	StringCchCopy(pBookmark->szItemDescription,
		SIZEOF_ARRAY(pBookmark->szItemDescription),
		pbi->szItemDescription);

	pBookmark->Type				= pbi->Type;
	pBookmark->bShowOnToolbar	= pbi->bShowOnToolbar;

	if(pbi->Type == BOOKMARK_TYPE_BOOKMARK)
	{
		/* If this is a bookmark, also copy
		across its location. The calling code
		will be responsible for freeing this. */
		StringCchCopy(pBookmark->szLocation,
			SIZEOF_ARRAY(pBookmark->szLocation),pbi->szLocation);
	}

	pBookmark->pHandle	= (void *)pbi;
}

void CBookmark::RetrieveBookmark(void *pBookmarkHandle,Bookmark_t *pBookmark)
{
	ExportBookmarkInternal((BookmarkInternal_t *)pBookmarkHandle,pBookmark);
}

void CBookmark::CreateNewBookmark(void *pParentHandle,Bookmark_t *pFolder)
{
	BookmarkInternal_t	*pbiParent = NULL;
	BookmarkInternal_t	*pNewBookmark = NULL;
	BookmarkInternal_t	*pChild = NULL;

	pbiParent = (BookmarkInternal_t *)pParentHandle;

	pNewBookmark = (BookmarkInternal_t *)malloc(sizeof(BookmarkInternal_t));

	ImportBookmarkInternal(pNewBookmark,pFolder);

	pNewBookmark->Parent = (void *)pbiParent;

	if(pbiParent->FirstChild == NULL)
	{
		pbiParent->FirstChild			= pNewBookmark;
		pNewBookmark->PreviousSibling	= NULL;
	}
	else
	{
		pChild = (BookmarkInternal_t *)pbiParent->FirstChild;

		while(pChild->NextSibling != NULL)
		{
			pChild = (BookmarkInternal_t *)pChild->NextSibling;
		}

		pChild->NextSibling				= pNewBookmark;
		pNewBookmark->PreviousSibling	= pChild;
	}
}

UINT Bookmark::m_IDCounter = 0;
UINT BookmarkFolder::m_IDCounter = 0;

Bookmark::Bookmark(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription) :
	m_ID(++m_IDCounter),
	m_strName(strName),
	m_strLocation(strLocation),
	m_strDescription(strDescription)
{
	GetSystemTimeAsFileTime(&m_ftCreated);
}

Bookmark::~Bookmark()
{

}

std::wstring Bookmark::GetName()
{
	return m_strName;
}

std::wstring Bookmark::GetLocation()
{
	return m_strLocation;
}

std::wstring Bookmark::GetDescription()
{
	return m_strDescription;
}

void Bookmark::SetName(const std::wstring &strName)
{
	m_strName = strName;
}

void Bookmark::SetLocation(const std::wstring &strLocation)
{
	m_strLocation = strLocation;
}

void Bookmark::SetDescription(const std::wstring &strDescription)
{
	m_strDescription = strDescription;
}

UINT Bookmark::GetID()
{
	return m_ID;
}

BookmarkFolder::BookmarkFolder(const std::wstring &strName) :
	m_ID(++m_IDCounter),
	m_strName(strName),
	m_nChildFolders(0)
{
	 GetSystemTimeAsFileTime(&m_ftCreated);
}

BookmarkFolder::~BookmarkFolder()
{

}

std::wstring BookmarkFolder::GetName()
{
	return m_strName;
}

void BookmarkFolder::SetName(const std::wstring &strName)
{
	m_strName = strName;
}

UINT BookmarkFolder::GetID()
{
	return m_ID;
}

void BookmarkFolder::InsertBookmark(const Bookmark &bm)
{
	InsertBookmark(bm,m_ChildList.size());
}

void BookmarkFolder::InsertBookmark(const Bookmark &bm,std::size_t Position)
{
	if(Position > (m_ChildList.size() - 1))
	{
		m_ChildList.push_back(bm);
	}
	else
	{
		auto itr = m_ChildList.begin();
		std::advance(itr,Position);
		m_ChildList.insert(itr,bm);
	}
}

void BookmarkFolder::InsertBookmarkFolder(const BookmarkFolder &bf)
{
	InsertBookmarkFolder(bf,m_ChildList.size());
}

void BookmarkFolder::InsertBookmarkFolder(const BookmarkFolder &bf,std::size_t Position)
{
	if(Position > (m_ChildList.size() - 1))
	{
		m_ChildList.push_back(bf);
	}
	else
	{
		auto itr = m_ChildList.begin();
		std::advance(itr,Position);
		m_ChildList.insert(itr,bf);
	}

	m_nChildFolders++;
}

std::list<boost::variant<BookmarkFolder,Bookmark>>::iterator BookmarkFolder::begin()
{
	return m_ChildList.begin();
}

std::list<boost::variant<BookmarkFolder,Bookmark>>::iterator BookmarkFolder::end()
{
	return m_ChildList.end();
}

bool BookmarkFolder::HasChildFolder()
{
	if(m_nChildFolders > 0)
	{
		return true;
	}

	return false;
}

std::pair<void *,NBookmarks::BookmarkType_t> BookmarkFolder::GetBookmarkItem(UINT uID)
{
	auto itr = std::find_if(m_ChildList.begin(),m_ChildList.end(),
		[uID](boost::variant<BookmarkFolder,Bookmark> &Variant) -> BOOL
		{
			BookmarkFolder *pBookmarkFolder = boost::get<BookmarkFolder>(&Variant);

			if(pBookmarkFolder)
			{
				return pBookmarkFolder->GetID() == uID;
			}

			return FALSE;
		}
	);

	if(itr != m_ChildList.end())
	{
		return std::make_pair(reinterpret_cast<void *>(boost::get<BookmarkFolder>(&(*itr))),
			NBookmarks::TYPE_BOOKMARK);
	}

	return std::make_pair(reinterpret_cast<void *>(NULL),NBookmarks::TYPE_BOOKMARK);
}