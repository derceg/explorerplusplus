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
#include "RegistrySettings.h"
#include "Helper.h"


UINT CBookmark::m_IDCounter = 0;
UINT CBookmarkFolder::m_IDCounter = 0;

CBookmark::CBookmark(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription) :
	m_ID(++m_IDCounter),
	m_strName(strName),
	m_strLocation(strLocation),
	m_strDescription(strDescription),
	m_iVisitCount(0)
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

CBookmarkFolder CBookmarkFolder::Create(const std::wstring &strName)
{
	return CBookmarkFolder(strName,INITIALIZATION_TYPE_NORMAL);
}

CBookmarkFolder *CBookmarkFolder::CreateNew(const std::wstring &strName)
{
	return new CBookmarkFolder(strName,INITIALIZATION_TYPE_NORMAL);
}

CBookmarkFolder CBookmarkFolder::UnserializeFromRegistry(const std::wstring &strKey)
{
	return CBookmarkFolder(strKey,INITIALIZATION_TYPE_REGISTRY);
}

CBookmarkFolder::CBookmarkFolder(const std::wstring &str,InitializationType_t InitializationType)
{
	switch(InitializationType)
	{
	case INITIALIZATION_TYPE_REGISTRY:
		InitializeFromRegistry(str);
		break;

	default:
		Initialize(str);
		break;
	}
}

CBookmarkFolder::~CBookmarkFolder()
{

}

void CBookmarkFolder::Initialize(const std::wstring &strName)
{
	m_ID = ++m_IDCounter;
	m_strName = strName;
	m_nChildFolders = 0;

	GetSystemTimeAsFileTime(&m_ftCreated);
}

void CBookmarkFolder::InitializeFromRegistry(const std::wstring &strKey)
{
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,strKey.c_str(),0,KEY_READ,&hKey);

	if(lRes == ERROR_SUCCESS)
	{
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("ID"),reinterpret_cast<DWORD *>(&m_ID));
		NRegistrySettings::ReadStringFromRegistry(hKey,_T("Name"),m_strName);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateCreatedLow"),&m_ftCreated.dwLowDateTime);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateCreatedHigh"),&m_ftCreated.dwHighDateTime);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateModifiedLow"),&m_ftModified.dwLowDateTime);
		NRegistrySettings::ReadDwordFromRegistry(hKey,_T("DateModifiedHigh"),&m_ftModified.dwHighDateTime);

		TCHAR szSubKeyName[256];
		DWORD dwSize = SIZEOF_ARRAY(szSubKeyName);
		int iIndex = 0;

		while(RegEnumKeyEx(hKey,iIndex,szSubKeyName,&dwSize,NULL,NULL,NULL,NULL) == ERROR_SUCCESS)
		{
			TCHAR szSubKey[256];
			StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\%s"),strKey.c_str(),szSubKeyName);

			if(CheckWildcardMatch(_T("BookmarkFolder_*"),szSubKeyName,FALSE))
			{
				CBookmarkFolder BookmarkFolder = CBookmarkFolder::UnserializeFromRegistry(szSubKey);
				m_ChildList.push_back(BookmarkFolder);
			}
			else if(CheckWildcardMatch(_T("Bookmark_*"),szSubKeyName,FALSE))
			{
				/* TODO: Create bookmark. */
			}

			dwSize = SIZEOF_ARRAY(szSubKeyName);
			iIndex++;
		}

		RegCloseKey(hKey);
	}
}

void CBookmarkFolder::SerializeToRegistry(const std::wstring &strKey)
{
	HKEY hKey;
	LONG lRes = RegCreateKeyEx(HKEY_CURRENT_USER,strKey.c_str(),
	0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,NULL);

	if(lRes == ERROR_SUCCESS)
	{
		/* These details don't need to be saved for the root bookmark. */
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("ID"),m_ID);
		NRegistrySettings::SaveStringToRegistry(hKey,_T("Name"),m_strName.c_str());
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateCreatedLow"),m_ftCreated.dwLowDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateCreatedHigh"),m_ftCreated.dwHighDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateModifiedLow"),m_ftModified.dwLowDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateModifiedHigh"),m_ftModified.dwHighDateTime);

		int iItem = 0;

		for each(auto Variant in m_ChildList)
		{
			TCHAR szSubKey[256];

			if(CBookmarkFolder *pBookmarkFolder = boost::get<CBookmarkFolder>(&Variant))
			{
				StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\BookmarkFolder_%d"),strKey.c_str(),iItem);
				pBookmarkFolder->SerializeToRegistry(szSubKey);
			}
			else if(CBookmark *pBookmark = boost::get<CBookmark>(&Variant))
			{
				StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),_T("%s\\Bookmark_%d"),strKey.c_str(),iItem);

				/* TODO: Serialize. */
			}

			iItem++;
		}

		RegCloseKey(hKey);
	}
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