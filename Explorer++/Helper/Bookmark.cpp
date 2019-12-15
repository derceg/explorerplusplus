// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmark.h"
#include "Helper.h"
#include "Macros.h"
#include "StringHelper.h"
#include <algorithm>
#include <list>

CBookmark CBookmark::Create(const std::wstring &strName, const std::wstring &strLocation, const std::wstring &strDescription)
{
	return CBookmark(strName, strLocation, strDescription);
}

CBookmark::CBookmark(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription) :
	m_guid(CreateGUID()),
	m_strName(strName),
	m_strLocation(strLocation),
	m_strDescription(strDescription),
	m_iVisitCount(0)
{
	GetSystemTimeAsFileTime(&m_ftCreated);
	m_ftModified = m_ftCreated;
}

std::wstring CBookmark::GetName() const
{
	return m_strName;
}

std::wstring CBookmark::GetLocation() const
{
	return m_strLocation;
}

std::wstring CBookmark::GetDescription() const
{
	return m_strDescription;
}

void CBookmark::SetName(const std::wstring &strName)
{
	m_strName = strName;

	UpdateModificationTime();
}

void CBookmark::SetLocation(const std::wstring &strLocation)
{
	m_strLocation = strLocation;

	UpdateModificationTime();
}

void CBookmark::SetDescription(const std::wstring &strDescription)
{
	m_strDescription = strDescription;

	UpdateModificationTime();
}

std::wstring CBookmark::GetGUID() const
{
	return m_guid;
}

int CBookmark::GetVisitCount() const
{
	return m_iVisitCount;
}

FILETIME CBookmark::GetDateLastVisited() const
{
	return m_ftLastVisited;
}

void CBookmark::UpdateVisitCount()
{
	++m_iVisitCount;
	GetSystemTimeAsFileTime(&m_ftLastVisited);
}

FILETIME CBookmark::GetDateCreated() const
{
	return m_ftCreated;
}

FILETIME CBookmark::GetDateModified() const
{
	return m_ftModified;
}

void CBookmark::UpdateModificationTime()
{
	GetSystemTimeAsFileTime(&m_ftModified);
}

CBookmarkFolder CBookmarkFolder::Create(const std::wstring &strName, std::optional<std::wstring> guid)
{
	return CBookmarkFolder(strName, INITIALIZATION_TYPE_NORMAL, guid);
}

CBookmarkFolder *CBookmarkFolder::CreateNew(const std::wstring &strName, std::optional<std::wstring> guid)
{
	return new CBookmarkFolder(strName, INITIALIZATION_TYPE_NORMAL, guid);
}

CBookmarkFolder::CBookmarkFolder(const std::wstring &str, InitializationType_t InitializationType,
	std::optional<std::wstring> guid)
{
	switch(InitializationType)
	{
	default:
		Initialize(str,guid);
		break;
	}
}

void CBookmarkFolder::Initialize(const std::wstring &name, std::optional<std::wstring> guid)
{
	if(guid)
	{
		m_guid = *guid;
	}
	else
	{
		m_guid = CreateGUID();
	}

	m_strName = name;
	m_nChildFolders = 0;

	GetSystemTimeAsFileTime(&m_ftCreated);

	m_ftModified = m_ftCreated;
}

std::wstring CBookmarkFolder::GetName() const
{
	return m_strName;
}

void CBookmarkFolder::SetName(const std::wstring &strName)
{
	m_strName = strName;

	UpdateModificationTime();
}

std::wstring CBookmarkFolder::GetGUID() const
{
	return m_guid;
}

FILETIME CBookmarkFolder::GetDateCreated() const
{
	return m_ftCreated;
}

FILETIME CBookmarkFolder::GetDateModified() const
{
	return m_ftModified;
}

void CBookmarkFolder::UpdateModificationTime()
{
	GetSystemTimeAsFileTime(&m_ftModified);
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

	GetSystemTimeAsFileTime(&m_ftModified);
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

	GetSystemTimeAsFileTime(&m_ftModified);
}

std::list<VariantBookmark>::iterator CBookmarkFolder::begin()
{
	return m_ChildList.begin();
}

std::list<VariantBookmark>::iterator CBookmarkFolder::end()
{
	return m_ChildList.end();
}

std::list<VariantBookmark>::const_iterator CBookmarkFolder::begin() const
{
	return m_ChildList.begin();
}

std::list<VariantBookmark>::const_iterator CBookmarkFolder::end() const
{
	return m_ChildList.end();
}

bool CBookmarkFolder::HasChildren() const
{
	return !m_ChildList.empty();
}

bool CBookmarkFolder::HasChildFolder() const
{
	if(m_nChildFolders > 0)
	{
		return true;
	}

	return false;
}