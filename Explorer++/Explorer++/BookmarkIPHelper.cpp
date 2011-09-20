/******************************************************************
 *
 * Project: Explorer++
 * File: BookmarkIPHelper.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides inter-process bookmark notification functionality.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "BookmarkIPHelper.h"
#include "../Helper/Macros.h"


struct IPBookmarkAddedNotification_t
{
	NExplorerplusplus::IPNotificationType_t Type;

	GUID guidParent;
	UINT uPosition;
};

struct IPBookmarkModifiedNotification_t
{
	NExplorerplusplus::IPNotificationType_t Type;
};

struct IPBookmarkRemovedNotification_t
{
	NExplorerplusplus::IPNotificationType_t Type;

	GUID guid;
};

CIPBookmarkItemNotifier::CIPBookmarkItemNotifier(HWND hTopLevelWnd,
	NBookmarkIPHelper::IPBookmarkNotificationGet *pipbng) :
m_hTopLevelWnd(hTopLevelWnd),
m_pipbng(pipbng)
{

}

CIPBookmarkItemNotifier::~CIPBookmarkItemNotifier()
{

}

BOOL CALLBACK BookmarkNotifierEnumWindowsStub(HWND hwnd,LPARAM lParam)
{
	CIPBookmarkItemNotifier::EnumerationData_t *ped = reinterpret_cast<CIPBookmarkItemNotifier::EnumerationData_t *>(lParam);

	return ped->pipbin->BookmarkNotifierEnumWindows(hwnd,ped->pData,ped->uSize);
}

BOOL CALLBACK CIPBookmarkItemNotifier::BookmarkNotifierEnumWindows(HWND hwnd,void *pData,UINT uSize)
{
	TCHAR szClassName[256];
	int iRes = GetClassName(hwnd,szClassName,SIZEOF_ARRAY(szClassName));

	if(iRes != 0 &&
		lstrcmp(szClassName,NExplorerplusplus::CLASS_NAME) == 0 &&
		hwnd != m_hTopLevelWnd &&
		m_pipbng->GetIPBroadcast())
	{
		COPYDATASTRUCT cds;
		cds.lpData = pData;
		cds.cbData = uSize;
		cds.dwData = NULL;
		SendMessage(hwnd,WM_COPYDATA,reinterpret_cast<WPARAM>(m_hTopLevelWnd),reinterpret_cast<LPARAM>(&cds));
	}

	return TRUE;
}

void CIPBookmarkItemNotifier::OnBookmarkModified(const GUID &guid)
{
	/* TODO: Need to be able to look up the specified bookmark and
	serialize it. */
	//BookmarkItemModified(sd,false);
}

void CIPBookmarkItemNotifier::OnBookmarkFolderModified(const GUID &guid)
{
	/* TODO: Serialize. */
	//BookmarkItemModified(,true);
}

void CIPBookmarkItemNotifier::BookmarkItemModified(const NBookmark::SerializedData_t &sd,bool bFolder)
{
	IPBookmarkModifiedNotification_t ipbmn;

	if(bFolder)
	{
		ipbmn.Type = NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_FOLDER_MODIFIED;
	}
	else
	{
		ipbmn.Type = NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_MODIFIED;
	}

	EnumerationData_t ed = SetupIPData(&ipbmn,sizeof(ipbmn),sd.pData,sd.uSize);

	EnumWindows(BookmarkNotifierEnumWindowsStub,reinterpret_cast<LPARAM>(&ed));

	delete ed.pData;
}

void CIPBookmarkItemNotifier::OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmark &Bookmark,std::size_t Position)
{
	NBookmark::SerializedData_t sd = Bookmark.Serialize();
	BookmarkItemAdded(ParentBookmarkFolder,sd,Position,false);

	delete sd.pData;
}

void CIPBookmarkItemNotifier::OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const CBookmarkFolder &BookmarkFolder,std::size_t Position)
{
	NBookmark::SerializedData_t sd = BookmarkFolder.Serialize();
	BookmarkItemAdded(ParentBookmarkFolder,sd,Position,true);

	delete sd.pData;
}

void CIPBookmarkItemNotifier::BookmarkItemAdded(const CBookmarkFolder &ParentBookmarkFolder,
	const NBookmark::SerializedData_t &sd,std::size_t Position,bool bFolder)
{
	IPBookmarkAddedNotification_t ipban;

	if(bFolder)
	{
		ipban.Type = NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_FOLDER_ADDED;
	}
	else
	{
		ipban.Type = NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_ADDED;
	}

	ipban.guidParent = ParentBookmarkFolder.GetGUID();
	ipban.uPosition = static_cast<UINT>(Position);

	EnumerationData_t ed = SetupIPData(&ipban,sizeof(ipban),sd.pData,sd.uSize);

	EnumWindows(BookmarkNotifierEnumWindowsStub,reinterpret_cast<LPARAM>(&ed));

	delete ed.pData;
}

void CIPBookmarkItemNotifier::OnBookmarkRemoved(const GUID &guid)
{
	BookmarkItemRemoved(guid,false);
}

void CIPBookmarkItemNotifier::OnBookmarkFolderRemoved(const GUID &guid)
{
	BookmarkItemRemoved(guid,true);
}

void CIPBookmarkItemNotifier::BookmarkItemRemoved(const GUID &guid,bool bFolder)
{
	IPBookmarkRemovedNotification_t ipbrn;

	if(bFolder)
	{
		ipbrn.Type = NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_FOLDER_REMOVED;
	}
	else
	{
		ipbrn.Type = NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_REMOVED;
	}

	ipbrn.guid = guid;

	EnumerationData_t ed = SetupIPData(&ipbrn,sizeof(ipbrn),NULL,0);

	EnumWindows(BookmarkNotifierEnumWindowsStub,reinterpret_cast<LPARAM>(&ed));

	delete ed.pData;
}

CIPBookmarkItemNotifier::EnumerationData_t CIPBookmarkItemNotifier::SetupIPData(
	void *pNotification,UINT uNotificationSize,
	void *pSerializedBookmarkItem,UINT uBookmarkItemSize)
{
	char *pData = new char[uNotificationSize + uBookmarkItemSize];
	memcpy(pData,pNotification,uNotificationSize);
	memcpy(pData + uNotificationSize,pSerializedBookmarkItem,uBookmarkItemSize);

	EnumerationData_t ed;
	ed.pipbin	= this;
	ed.pData	= pData;
	ed.uSize	= uNotificationSize + uBookmarkItemSize;

	return ed;
}

CIPBookmarkObserver::CIPBookmarkObserver(CBookmarkFolder *pAllBookmarks,
	NBookmarkIPHelper::IPBookmarkNotificationSet *pipbns) :
m_pAllBookmarks(pAllBookmarks),
m_pipbns(pipbns)
{
	 
}

CIPBookmarkObserver::~CIPBookmarkObserver()
{

}

void CIPBookmarkObserver::OnNotificationReceived(NExplorerplusplus::IPNotificationType_t ipnt,void *pData)
{
	switch(ipnt)
	{
	case NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_ADDED:
		BookmarkItemAdded(pData,false);
		break;

	case NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_FOLDER_ADDED:
		BookmarkItemAdded(pData,true);
		break;

	case NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_MODIFIED:
		break;

	case NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_FOLDER_MODIFIED:
		break;

	case NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_REMOVED:
		break;

	case NExplorerplusplus::IP_NOTIFICATION_TYPE_BOOKMARK_FOLDER_REMOVED:
		break;
	}
}

void CIPBookmarkObserver::BookmarkItemAdded(void *pData,bool bFolder)
{
	IPBookmarkAddedNotification_t *pipban = reinterpret_cast<IPBookmarkAddedNotification_t *>(pData);

	/* TODO: Find the parent with the specified GUID. */
	//pipban->guidParent
	CBookmarkFolder *pParentBookmarkFolder = m_pAllBookmarks;

	m_pipbns->SetIPBroadcast(false);

	char *pSerializedData = reinterpret_cast<char *>(pData) + sizeof(IPBookmarkAddedNotification_t);

	try
	{
		if(bFolder)
		{
			CBookmarkFolder BookmarkFolder = CBookmarkFolder::Unserialize(pSerializedData);
			pParentBookmarkFolder->InsertBookmarkFolder(BookmarkFolder,pipban->uPosition);
		}
		else
		{
			CBookmark Bookmark(pSerializedData);
			pParentBookmarkFolder->InsertBookmark(Bookmark,pipban->uPosition);
		}
	}
	catch(int)
	{
		/* The actual content of the error here
		is unimportant. May be needed in the
		future if an error message will be shown
		to the user. */
	}

	m_pipbns->SetIPBroadcast(true);
}

void CIPBookmarkObserver::BookmarkItemModified(void *pData)
{
	//IPBookmarkModifiedNotification_t *pipbmn = reinterpret_cast<IPBookmarkModifiedNotification_t *>(pData);

	/* TODO: Find the bookmark and modify it. */
}

void CIPBookmarkObserver::BookmarkItemRemoved(void *pData)
{
	//IPBookmarkRemovedNotification_t *pipbrn = reinterpret_cast<IPBookmarkRemovedNotification_t *>(pData);

	/* TODO: Find the bookmarks parent, and remove the
	item. */
}

bool Explorerplusplus::GetIPBroadcast() const
{
	return m_bBroadcastIPBookmarkNotifications;
}

void Explorerplusplus::SetIPBroadcast(bool bBroadcast)
{
	m_bBroadcastIPBookmarkNotifications = bBroadcast;
}