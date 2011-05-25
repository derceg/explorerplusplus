/******************************************************************
 *
 * Project: Helper
 * File: Bookmark.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Implements a bookmark system, with both bookmark folders
 * and bookmarks.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include <algorithm>
#include "Bookmark.h"


UINT CBookmark::m_IDCounter = 0;
UINT CBookmarkFolder::m_IDCounter = 0;

CBookmark::CBookmark(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription) :
	m_ID(++m_IDCounter),
	m_strName(strName),
	m_strLocation(strLocation),
	m_strDescription(strDescription)
{
	GetSystemTimeAsFileTime(&m_ftCreated);
}

CBookmark::~CBookmark()
{

}

std::wstring CBookmark::GetName()
{
	return m_strName;
}

std::wstring CBookmark::GetLocation()
{
	return m_strLocation;
}

std::wstring CBookmark::GetDescription()
{
	return m_strDescription;
}

void CBookmark::SetName(const std::wstring &strName)
{
	m_strName = strName;
}

void CBookmark::SetLocation(const std::wstring &strLocation)
{
	m_strLocation = strLocation;
}

void CBookmark::SetDescription(const std::wstring &strDescription)
{
	m_strDescription = strDescription;
}

UINT CBookmark::GetID()
{
	return m_ID;
}

CBookmarkFolder::CBookmarkFolder(const std::wstring &strName) :
	m_ID(++m_IDCounter),
	m_strName(strName),
	m_nChildFolders(0)
{
	 GetSystemTimeAsFileTime(&m_ftCreated);
}

CBookmarkFolder::~CBookmarkFolder()
{

}

std::wstring CBookmarkFolder::GetName()
{
	return m_strName;
}

void CBookmarkFolder::SetName(const std::wstring &strName)
{
	m_strName = strName;
}

UINT CBookmarkFolder::GetID()
{
	return m_ID;
}

void CBookmarkFolder::InsertBookmark(const CBookmark &Bookmark)
{
	InsertBookmark(Bookmark,m_ChildList.size());
}

void CBookmarkFolder::InsertBookmark(const CBookmark &Bookmark,std::size_t Position)
{
	if(Position > (m_ChildList.size() - 1))
	{
		m_ChildList.push_back(Bookmark);
	}
	else
	{
		auto itr = m_ChildList.begin();
		std::advance(itr,Position);
		m_ChildList.insert(itr,Bookmark);
	}
}

void CBookmarkFolder::InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder)
{
	InsertBookmarkFolder(BookmarkFolder,m_ChildList.size());
}

void CBookmarkFolder::InsertBookmarkFolder(const CBookmarkFolder &BookmarkFolder,std::size_t Position)
{
	if(Position > (m_ChildList.size() - 1))
	{
		m_ChildList.push_back(BookmarkFolder);
	}
	else
	{
		auto itr = m_ChildList.begin();
		std::advance(itr,Position);
		m_ChildList.insert(itr,BookmarkFolder);
	}

	m_nChildFolders++;
}

std::list<boost::variant<CBookmarkFolder,CBookmark>>::iterator CBookmarkFolder::begin()
{
	return m_ChildList.begin();
}

std::list<boost::variant<CBookmarkFolder,CBookmark>>::iterator CBookmarkFolder::end()
{
	return m_ChildList.end();
}

bool CBookmarkFolder::HasChildFolder()
{
	if(m_nChildFolders > 0)
	{
		return true;
	}

	return false;
}

std::pair<void *,NBookmarks::BookmarkType_t> CBookmarkFolder::GetBookmarkItem(UINT uID)
{
	auto itr = std::find_if(m_ChildList.begin(),m_ChildList.end(),
		[uID](boost::variant<CBookmarkFolder,CBookmark> &Variant) -> BOOL
		{
			CBookmarkFolder *pBookmarkFolder = boost::get<CBookmarkFolder>(&Variant);

			if(pBookmarkFolder)
			{
				return pBookmarkFolder->GetID() == uID;
			}

			return FALSE;
		}
	);

	if(itr != m_ChildList.end())
	{
		return std::make_pair(reinterpret_cast<void *>(boost::get<CBookmarkFolder>(&(*itr))),
			NBookmarks::TYPE_BOOKMARK);
	}

	return std::make_pair(reinterpret_cast<void *>(NULL),NBookmarks::TYPE_BOOKMARK);
}