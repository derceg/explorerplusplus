#ifndef BOOKMARKIPHELPER_INCLUDED
#define BOOKMARKIPHELPER_INCLUDED

#include "Explorer++_internal.h"
#include "../Helper/Bookmark.h"

namespace NBookmarkIPHelper
{
	__interface IPBookmarkNotificationGet
	{
		bool GetIPBroadcast() const;
	};

	__interface IPBookmarkNotificationSet
	{
		void SetIPBroadcast(bool bBroadcast);
	};
}

/* Receives low-level bookmark notifications, and rebroadcasts them
via IPC to other Explorer++ processes. */
class CIPBookmarkItemNotifier : public NBookmark::IBookmarkItemNotification
{
	friend BOOL CALLBACK BookmarkNotifierEnumWindowsStub(HWND hwnd,LPARAM lParam);

public:

	CIPBookmarkItemNotifier(HWND hTopLevelWnd,NBookmarkIPHelper::IPBookmarkNotificationGet *pipbng);
	~CIPBookmarkItemNotifier();

	void	OnBookmarkAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmark &Bookmark,std::size_t Position);
	void	OnBookmarkFolderAdded(const CBookmarkFolder &ParentBookmarkFolder,const CBookmarkFolder &BookmarkFolder,std::size_t Position);
	void	OnBookmarkModified(const GUID &guid);
	void	OnBookmarkFolderModified(const GUID &guid);
	void	OnBookmarkRemoved(const GUID &guid);
	void	OnBookmarkFolderRemoved(const GUID &guid);

private:

	struct EnumerationData_t
	{
		CIPBookmarkItemNotifier	*pipbin;

		void *pData;
		UINT uSize;
	};

	void				BookmarkItemAdded(const CBookmarkFolder &ParentBookmarkFolder,const NBookmark::SerializedData_t &sd,std::size_t Position,bool bFolder);
	void				BookmarkItemModified(const NBookmark::SerializedData_t &sd,bool bFolder);
	void				BookmarkItemRemoved(const GUID &guid,bool bFolder);

	BOOL CALLBACK		BookmarkNotifierEnumWindows(HWND hwnd,void *pData,UINT uSize);
	EnumerationData_t	SetupIPData(void *pNotification,UINT uNotificationSize,void *pSerializedBookmarkItem,UINT uBookmarkItemSize);

	HWND				m_hTopLevelWnd;

	NBookmarkIPHelper::IPBookmarkNotificationGet *m_pipbng;
};

/* Receives bookmark notifications via IPC from other Explorer++ process,
and rebroadcasts those notifications internally.
This class emulates all bookmark notifications. That is, upon receiving a
modification, addition, etc notification, this class will reconstruct the
changes locally. This will then cause the changes to be rebroadcast internally.
While reconstructing the changes, this class sets a flag indicating that
the changes are not to rebroadcast. */
class CIPBookmarkObserver
{
public:

	CIPBookmarkObserver(CBookmarkFolder *pAllBookmarks,NBookmarkIPHelper::IPBookmarkNotificationSet *pipbns);
	~CIPBookmarkObserver();

	void	OnNotificationReceived(NExplorerplusplus::IPNotificationType_t ipnt,void *pData);

private:

	void	BookmarkItemAdded(void *pData,bool bFolder);
	void	BookmarkItemModified(void *pData);
	void	BookmarkItemRemoved(void *pData);

	CBookmarkFolder *m_pAllBookmarks;

	NBookmarkIPHelper::IPBookmarkNotificationSet *m_pipbns;
};

#endif