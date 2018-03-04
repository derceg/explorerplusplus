/******************************************************************
 *
 * Project: Helper
 * File: Bookmark.cpp
 * License: GPL - See LICENSE in the top level directory
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
#include "StringHelper.h"
#include "Macros.h"


CBookmark::CBookmark(const std::wstring &strName,const std::wstring &strLocation,const std::wstring &strDescription) :
	m_strName(strName),
	m_strLocation(strLocation),
	m_strDescription(strDescription),
	m_iVisitCount(0)
{
	CoCreateGuid(&m_guid);
	GetSystemTimeAsFileTime(&m_ftCreated);
}

CBookmark::CBookmark(HKEY hKey, TCHAR *szKey)
{
	BookmarkSerialized_t *pbs = new BookmarkSerialized_t;
	int result = NRegistrySettings::ReadBinFromRegistry(hKey, szKey, 
		reinterpret_cast<LPBYTE>(pbs), sizeof(BookmarkSerialized_t));

	if (result != ERROR_SUCCESS)
		return;

	if (pbs->uSize != sizeof(BookmarkSerialized_t))
	{
		throw NBookmark::VERSION_NUMBER_MISMATCH;
	}

	m_guid = pbs->guid;

	m_strName = pbs->Name;
	m_strLocation = pbs->Location;
	m_strDescription = pbs->Description;

	m_iVisitCount = pbs->iVisitCount;
	m_ftLastVisited = pbs->ftLastVisited;

	m_ftCreated = pbs->ftCreated;
	m_ftModified = pbs->ftModified;
}

/* TODO: Transform to named constructor. */
CBookmark::CBookmark(void *pSerializedData)
{
	const BookmarkSerialized_t *pbs = reinterpret_cast<BookmarkSerialized_t *>(pSerializedData);

	if(pbs->uSize != sizeof(BookmarkSerialized_t))
	{
		throw NBookmark::VERSION_NUMBER_MISMATCH;
	}

	m_guid = pbs->guid;

	m_strName = pbs->Name;
	m_strLocation = pbs->Location;
	m_strDescription = pbs->Description;

	m_iVisitCount = pbs->iVisitCount;
	m_ftLastVisited = pbs->ftLastVisited;

	m_ftCreated = pbs->ftCreated;
	m_ftModified = pbs->ftModified;
}

CBookmark::~CBookmark()
{

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

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkModified(m_guid);
}

void CBookmark::SetLocation(const std::wstring &strLocation)
{
	m_strLocation = strLocation;

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkModified(m_guid);
}

void CBookmark::SetDescription(const std::wstring &strDescription)
{
	m_strDescription = strDescription;

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkModified(m_guid);
}

GUID CBookmark::GetGUID() const
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

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkModified(m_guid);
}

FILETIME CBookmark::GetDateCreated() const
{
	return m_ftCreated;
}

FILETIME CBookmark::GetDateModified() const
{
	return m_ftModified;
}

NBookmark::SerializedData_t CBookmark::Serialize() const
{
	/* Save ALL the properties for this bookmark into
	a structure, and return that structure. */
	BookmarkSerialized_t *pbs = new BookmarkSerialized_t;
	ZeroMemory(pbs, sizeof(BookmarkSerialized_t));

	pbs->uSize = sizeof(BookmarkSerialized_t);

	pbs->guid = m_guid;

	StringCchCopy(pbs->Name,SIZEOF_ARRAY(pbs->Name),m_strName.c_str());
	StringCchCopy(pbs->Location,SIZEOF_ARRAY(pbs->Location),m_strLocation.c_str());
	StringCchCopy(pbs->Description,SIZEOF_ARRAY(pbs->Description),m_strDescription.c_str());

	pbs->iVisitCount = m_iVisitCount;
	pbs->ftLastVisited = m_ftLastVisited;

	pbs->ftCreated = m_ftCreated;
	pbs->ftModified = m_ftModified;

	NBookmark::SerializedData_t sd;
	sd.pData = pbs;
	sd.uSize = sizeof(BookmarkSerialized_t);

	return sd;
}

CBookmarkFolder CBookmarkFolder::Create(const std::wstring &strName,GUID &guid)
{
	return CBookmarkFolder(strName,INITIALIZATION_TYPE_NORMAL,&guid);
}

CBookmarkFolder CBookmarkFolder::Create(const std::wstring &strName)
{
	return CBookmarkFolder(strName,INITIALIZATION_TYPE_NORMAL,NULL);
}

CBookmarkFolder *CBookmarkFolder::CreateNew(const std::wstring &strName,GUID &guid)
{
	return new CBookmarkFolder(strName,INITIALIZATION_TYPE_NORMAL,&guid);
}

CBookmarkFolder *CBookmarkFolder::CreateNew(const std::wstring &strName)
{
	return new CBookmarkFolder(strName,INITIALIZATION_TYPE_NORMAL,NULL);
}

CBookmarkFolder CBookmarkFolder::Unserialize(void *pSerializedData)
{
	return CBookmarkFolder(pSerializedData);
}

CBookmarkFolder CBookmarkFolder::UnserializeFromRegistry(const std::wstring &strKey)
{
	return CBookmarkFolder(strKey,INITIALIZATION_TYPE_REGISTRY, NULL);
}

CBookmarkFolder::CBookmarkFolder(const std::wstring &str,InitializationType_t InitializationType,GUID *guid)
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

CBookmarkFolder::CBookmarkFolder(void *pSerializedData)
{
	const BookmarkFolderSerialized_t *pbfs = reinterpret_cast<BookmarkFolderSerialized_t *>(pSerializedData);

	if(pbfs->uSize != sizeof(BookmarkFolderSerialized_t))
	{
		throw NBookmark::VERSION_NUMBER_MISMATCH;
	}

	m_guid = pbfs->guid;

	m_strName = pbfs->Name;

	m_ftCreated = pbfs->ftCreated;
	m_ftModified = pbfs->ftModified;
}

CBookmarkFolder::~CBookmarkFolder()
{

}

void CBookmarkFolder::Initialize(const std::wstring &strName,GUID *guid)
{
	if(guid != NULL)
	{
		m_guid = *guid;
	}
	else
	{
		CoCreateGuid(&m_guid);
	}

	m_strName = strName;
	m_nChildFolders = 0;

	GetSystemTimeAsFileTime(&m_ftCreated);

	m_ftModified = m_ftCreated;
}

void CBookmarkFolder::InitializeFromRegistry(const std::wstring &strKey)
{
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,strKey.c_str(),0,KEY_READ,&hKey);

	if (lRes != ERROR_SUCCESS)
		return;

	m_nChildFolders = 0;

	std::wstring stringGuid;
	NRegistrySettings::ReadStringFromRegistry(hKey, _T("GUID"), stringGuid);
	stringGuid = stringGuid.substr(1, stringGuid.length() - 2);

	TCHAR stringGuidTemp[128];
	StringCchCopy(stringGuidTemp, SIZEOF_ARRAY(stringGuidTemp), stringGuid.c_str());
	UuidFromString(reinterpret_cast<RPC_WSTR>(stringGuidTemp), &m_guid);

	NRegistrySettings::ReadStringFromRegistry(hKey, _T("Name"), m_strName);
	NRegistrySettings::ReadDwordFromRegistry(hKey, _T("DateCreatedLow"), &m_ftCreated.dwLowDateTime);
	NRegistrySettings::ReadDwordFromRegistry(hKey, _T("DateCreatedHigh"), &m_ftCreated.dwHighDateTime);
	NRegistrySettings::ReadDwordFromRegistry(hKey, _T("DateModifiedLow"), &m_ftModified.dwLowDateTime);
	NRegistrySettings::ReadDwordFromRegistry(hKey, _T("DateModifiedHigh"), &m_ftModified.dwHighDateTime);

	TCHAR szSubKeyName[256];
	DWORD dwSize = SIZEOF_ARRAY(szSubKeyName);
	int iIndex = 0;

	while (RegEnumKeyEx(hKey, iIndex, szSubKeyName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		TCHAR szSubKey[256];
		StringCchPrintf(szSubKey, SIZEOF_ARRAY(szSubKey), _T("%s\\%s"), strKey.c_str(), szSubKeyName);

		if (CheckWildcardMatch(_T("BookmarkFolder_*"), szSubKeyName, FALSE))
		{
			m_ChildList.push_back(CBookmarkFolder::UnserializeFromRegistry(szSubKey));
			m_nChildFolders++;
		}

		dwSize = SIZEOF_ARRAY(szSubKeyName);
		iIndex++;
	}

	iIndex = 0;
	while (RegEnumValue(hKey, iIndex, szSubKeyName, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		TCHAR szSubKey[256];
		StringCchPrintf(szSubKey, SIZEOF_ARRAY(szSubKey), _T("%s\\%s"), strKey.c_str(), szSubKeyName);

		if (CheckWildcardMatch(_T("Bookmark_*"), szSubKeyName, FALSE))
		{
			m_ChildList.push_back(CBookmark(hKey, szSubKeyName));
		}

		dwSize = SIZEOF_ARRAY(szSubKeyName);
		iIndex++;
	}
	RegCloseKey(hKey);
}

void CBookmarkFolder::SerializeToRegistry(const std::wstring &strKey)
{
	HKEY hKey;
	LONG lRes = RegCreateKeyEx(HKEY_CURRENT_USER,strKey.c_str(),
	0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,NULL);

	if (lRes != ERROR_SUCCESS)
		return;

	TCHAR guidString[128];
	StringFromGUID2(m_guid, guidString, SIZEOF_ARRAY(guidString));
	NRegistrySettings::SaveStringToRegistry(hKey, _T("GUID"), guidString);
	NRegistrySettings::SaveStringToRegistry(hKey, _T("Name"), m_strName.c_str());
	NRegistrySettings::SaveDwordToRegistry(hKey, _T("DateCreatedLow"), m_ftCreated.dwLowDateTime);
	NRegistrySettings::SaveDwordToRegistry(hKey, _T("DateCreatedHigh"), m_ftCreated.dwHighDateTime);
	NRegistrySettings::SaveDwordToRegistry(hKey, _T("DateModifiedLow"), m_ftModified.dwLowDateTime);
	NRegistrySettings::SaveDwordToRegistry(hKey, _T("DateModifiedHigh"), m_ftModified.dwHighDateTime);

	int iItem = 0;

	for each(auto Variant in m_ChildList)
	{
		TCHAR szSubKey[256];

		if (CBookmarkFolder *pBookmarkFolder = boost::get<CBookmarkFolder>(&Variant))
		{
			StringCchPrintf(szSubKey, SIZEOF_ARRAY(szSubKey), _T("%s\\BookmarkFolder_%d"), strKey.c_str(), iItem);
			pBookmarkFolder->SerializeToRegistry(szSubKey);
		}
		else if (CBookmark *pBookmark = boost::get<CBookmark>(&Variant))
		{
			StringCchPrintf(szSubKey, SIZEOF_ARRAY(szSubKey), _T("Bookmark_%d"), iItem);
			
			NBookmark::SerializedData_t data = pBookmark->Serialize();
			NRegistrySettings::SaveBinToRegistry(hKey, szSubKey, reinterpret_cast<LPBYTE>(data.pData), data.uSize);

			delete data.pData;
		}

		iItem++;
	}

	RegCloseKey(hKey);
}

std::wstring CBookmarkFolder::GetName() const
{
	return m_strName;
}

void CBookmarkFolder::SetName(const std::wstring &strName)
{
	m_strName = strName;

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkFolderModified(m_guid);
}

GUID CBookmarkFolder::GetGUID() const
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

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkAdded(*this,Bookmark,Position);
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

	CBookmarkItemNotifier::GetInstance().NotifyObserversBookmarkFolderAdded(*this,BookmarkFolder,Position);
}

std::list<boost::variant<CBookmarkFolder,CBookmark>>::iterator CBookmarkFolder::begin()
{
	return m_ChildList.begin();
}

std::list<boost::variant<CBookmarkFolder,CBookmark>>::iterator CBookmarkFolder::end()
{
	return m_ChildList.end();
}

std::list<boost::variant<CBookmarkFolder,CBookmark>>::const_iterator CBookmarkFolder::begin() const
{
	return m_ChildList.begin();
}

std::list<boost::variant<CBookmarkFolder,CBookmark>>::const_iterator CBookmarkFolder::end() const
{
	return m_ChildList.end();
}

bool CBookmarkFolder::HasChildFolder() const
{
	if(m_nChildFolders > 0)
	{
		return true;
	}

	return false;
}

NBookmark::SerializedData_t CBookmarkFolder::Serialize() const
{
	BookmarkFolderSerialized_t *pbfs = new BookmarkFolderSerialized_t;

	pbfs->uSize = sizeof(BookmarkFolderSerialized_t);

	pbfs->guid = m_guid;

	StringCchCopy(pbfs->Name,SIZEOF_ARRAY(pbfs->Name),m_strName.c_str());

	pbfs->ftCreated = m_ftCreated;
	pbfs->ftModified = m_ftModified;

	NBookmark::SerializedData_t sd;
	sd.pData = pbfs;
	sd.uSize = sizeof(BookmarkFolderSerialized_t);

	return sd;
}

CBookmarkItemNotifier::CBookmarkItemNotifier()
{

}

CBookmarkItemNotifier::~CBookmarkItemNotifier()
{

}

CBookmarkItemNotifier& CBookmarkItemNotifier::GetInstance()
{
	static CBookmarkItemNotifier bin;
	return bin;
}

void CBookmarkItemNotifier::AddObserver(NBookmark::IBookmarkItemNotification *pbin)
{
	m_listObservers.push_back(pbin);
}

void CBookmarkItemNotifier::RemoveObserver(NBookmark::IBookmarkItemNotification *pbin)
{
	auto itr = std::find_if(m_listObservers.begin(),m_listObservers.end(),
		[pbin](const NBookmark::IBookmarkItemNotification *pbinCurrent){return pbinCurrent == pbin;});

	if(itr != m_listObservers.end())
	{
		m_listObservers.erase(itr);
	}
}

void CBookmarkItemNotifier::NotifyObserversBookmarkModified(const GUID &guid)
{
	NotifyObservers(NOTIFY_BOOKMARK_MODIFIED,NULL,NULL,NULL,&guid,0);
}

void CBookmarkItemNotifier::NotifyObserversBookmarkFolderModified(const GUID &guid)
{
	NotifyObservers(NOTIFY_BOOKMARK_FOLDER_MODIFIED,NULL,NULL,NULL,&guid,0);
}

void CBookmarkItemNotifier::NotifyObserversBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmark &Bookmark,std::size_t Position)
{
	NotifyObservers(NOTIFY_BOOKMARK_ADDED,&ParentBookmarkFolder,NULL,&Bookmark,NULL,Position);
}

void CBookmarkItemNotifier::NotifyObserversBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmarkFolder &BookmarkFolder,std::size_t Position)
{
	NotifyObservers(NOTIFY_BOOKMARK_FOLDER_ADDED,&ParentBookmarkFolder,&BookmarkFolder,NULL,NULL,Position);
}

void CBookmarkItemNotifier::NotifyObserversBookmarkRemoved(const GUID &guid)
{
	NotifyObservers(NOTIFY_BOOKMARK_REMOVED,NULL,NULL,NULL,&guid,0);
}

void CBookmarkItemNotifier::NotifyObserversBookmarkFolderRemoved(const GUID &guid)
{
	NotifyObservers(NOTIFY_BOOMARK_FOLDER_REMOVED,NULL,NULL,NULL,&guid,0);
}

void CBookmarkItemNotifier::NotifyObservers(NotificationType_t NotificationType,
	const CBookmarkFolder *pParentBookmarkFolder,const CBookmarkFolder *pBookmarkFolder,
	const CBookmark *pBookmark,const GUID *pguid,std::size_t Position)
{
	for each(auto pbin in m_listObservers)
	{
		switch(NotificationType)
		{
		case NOTIFY_BOOKMARK_ADDED:
			pbin->OnBookmarkAdded(*pParentBookmarkFolder,*pBookmark,Position);
			break;

		case NOTIFY_BOOKMARK_FOLDER_ADDED:
			pbin->OnBookmarkFolderAdded(*pParentBookmarkFolder,*pBookmarkFolder,Position);
			break;

		case NOTIFY_BOOKMARK_MODIFIED:
			pbin->OnBookmarkModified(*pguid);
			break;

		case NOTIFY_BOOKMARK_FOLDER_MODIFIED:
			pbin->OnBookmarkFolderModified(*pguid);
			break;

		case NOTIFY_BOOKMARK_REMOVED:
			pbin->OnBookmarkRemoved(*pguid);
			break;

		case NOTIFY_BOOMARK_FOLDER_REMOVED:
			pbin->OnBookmarkFolderRemoved(*pguid);
			break;
		}
	}
}