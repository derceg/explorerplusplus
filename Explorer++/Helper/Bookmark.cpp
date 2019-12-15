// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmark.h"
#include "Helper.h"
#include "Macros.h"
#include "RegistrySettings.h"
#include "StringHelper.h"
#include <algorithm>
#include <list>

CBookmark CBookmark::Create(const std::wstring &strName, const std::wstring &strLocation, const std::wstring &strDescription)
{
	return CBookmark(strName, strLocation, strDescription);
}

CBookmark CBookmark::UnserializeFromRegistry(const std::wstring &strKey)
{
	return CBookmark(strKey);
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

CBookmark::CBookmark(const std::wstring &strKey)
{
	InitializeFromRegistry(strKey);
}

void CBookmark::InitializeFromRegistry(const std::wstring &strKey)
{
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER, strKey.c_str(), 0, KEY_READ, &hKey);

	if (lRes != ERROR_SUCCESS)
	{
		return;
	}

	NRegistrySettings::ReadStringFromRegistry(hKey, _T("GUID"), m_guid);
	NRegistrySettings::ReadStringFromRegistry(hKey, _T("Name"), m_strName);
	NRegistrySettings::ReadStringFromRegistry(hKey, _T("Location"), m_strLocation);
	NRegistrySettings::ReadStringFromRegistry(hKey, _T("Description"), m_strDescription);

	DWORD value;
	NRegistrySettings::ReadDwordFromRegistry(hKey, _T("VisitCount"), &value);
	m_iVisitCount = value;

	NRegistrySettings::ReadDwordFromRegistry(hKey, _T("DateLastVisitedLow"), &m_ftLastVisited.dwLowDateTime);
	NRegistrySettings::ReadDwordFromRegistry(hKey, _T("DateLastVisitedHigh"), &m_ftLastVisited.dwHighDateTime);

	NRegistrySettings::ReadDwordFromRegistry(hKey, _T("DateCreatedLow"), &m_ftCreated.dwLowDateTime);
	NRegistrySettings::ReadDwordFromRegistry(hKey, _T("DateCreatedHigh"), &m_ftCreated.dwHighDateTime);
	NRegistrySettings::ReadDwordFromRegistry(hKey, _T("DateModifiedLow"), &m_ftModified.dwLowDateTime);
	NRegistrySettings::ReadDwordFromRegistry(hKey, _T("DateModifiedHigh"), &m_ftModified.dwHighDateTime);
}

void CBookmark::SerializeToRegistry(const std::wstring &strKey)
{
	HKEY hKey;
	LONG lRes = RegCreateKeyEx(HKEY_CURRENT_USER, strKey.c_str(),
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);

	if (lRes == ERROR_SUCCESS)
	{
		NRegistrySettings::SaveStringToRegistry(hKey, _T("GUID"), m_guid.c_str());
		NRegistrySettings::SaveStringToRegistry(hKey, _T("Name"), m_strName.c_str());
		NRegistrySettings::SaveStringToRegistry(hKey, _T("Location"), m_strLocation.c_str());
		NRegistrySettings::SaveStringToRegistry(hKey, _T("Description"), m_strDescription.c_str());
		NRegistrySettings::SaveDwordToRegistry(hKey, _T("VisitCount"), m_iVisitCount);
		NRegistrySettings::SaveDwordToRegistry(hKey, _T("DateLastVisitedLow"), m_ftLastVisited.dwLowDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey, _T("DateLastVisitedHigh"), m_ftLastVisited.dwHighDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey, _T("DateCreatedLow"), m_ftCreated.dwLowDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey, _T("DateCreatedHigh"), m_ftCreated.dwHighDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey, _T("DateModifiedLow"), m_ftModified.dwLowDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey, _T("DateModifiedHigh"), m_ftModified.dwHighDateTime);

		RegCloseKey(hKey);
	}
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

CBookmarkFolder CBookmarkFolder::UnserializeFromRegistry(const std::wstring &strKey)
{
	return CBookmarkFolder(strKey, INITIALIZATION_TYPE_REGISTRY, std::nullopt);
}

CBookmarkFolder::CBookmarkFolder(const std::wstring &str, InitializationType_t InitializationType,
	std::optional<std::wstring> guid)
{
	switch(InitializationType)
	{
	case INITIALIZATION_TYPE_REGISTRY:
		InitializeFromRegistry(str);
		break;

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

void CBookmarkFolder::InitializeFromRegistry(const std::wstring &strKey)
{
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,strKey.c_str(),0,KEY_READ,&hKey);

	if(lRes == ERROR_SUCCESS)
	{
		NRegistrySettings::ReadStringFromRegistry(hKey,_T("GUID"), m_guid);
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
				CBookmark bookmark = CBookmark::UnserializeFromRegistry(szSubKey);
				m_ChildList.push_back(bookmark);
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
		NRegistrySettings::SaveStringToRegistry(hKey,_T("GUID"),m_guid.c_str());
		NRegistrySettings::SaveStringToRegistry(hKey,_T("Name"),m_strName.c_str());
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateCreatedLow"),m_ftCreated.dwLowDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateCreatedHigh"),m_ftCreated.dwHighDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateModifiedLow"),m_ftModified.dwLowDateTime);
		NRegistrySettings::SaveDwordToRegistry(hKey,_T("DateModifiedHigh"),m_ftModified.dwHighDateTime);

		int iItem = 0;

		for(auto Variant : m_ChildList)
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
				pBookmark->SerializeToRegistry(szSubKey);
			}

			iItem++;
		}

		RegCloseKey(hKey);
	}
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