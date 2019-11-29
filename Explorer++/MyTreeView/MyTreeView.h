// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/DropHandler.h"
#include "../Helper/iDirectoryMonitor.h"
#include "../Helper/ShellHelper.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <optional>

#define WM_USER_TREEVIEW				WM_APP + 70
#define WM_USER_TREEVIEW_GAINEDFOCUS	(WM_USER_TREEVIEW + 2)

class CMyTreeView : public IDropTarget, public IDropSource
{
public:

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

	CMyTreeView(HWND hTreeView, HWND hParent, IDirectoryMonitor *pDirMon);
	~CMyTreeView();

	/* Drop source functions. */
	HRESULT _stdcall	QueryContinueDrag(BOOL fEscapePressed,DWORD gfrKeyState);
	HRESULT _stdcall	GiveFeedback(DWORD dwEffect);

	LRESULT CALLBACK	TreeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	/* User functions. */
	HRESULT				AddDirectory(HTREEITEM hParent, PCIDLIST_ABSOLUTE pidlDirectory);
	unique_pidl_absolute	BuildPath(HTREEITEM hTreeItem);
	HTREEITEM			LocateItem(PCIDLIST_ABSOLUTE pidlDirectory);
	void				EraseItems(HTREEITEM hParent);
	BOOL				QueryDragging(void);
	void				SetShowHidden(BOOL bShowHidden);
	void				RefreshAllIcons(void);

	/* Sorting. */
	int CALLBACK		CompareItems(LPARAM lParam1,LPARAM lParam2);

	/* Drag and Drop. */
	HRESULT _stdcall	DragEnter(IDataObject *pDataObject,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragLeave(void);
	HRESULT _stdcall	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);

	void				MonitorDrivePublic(const TCHAR *szDrive);

private:

	static const UINT WM_APP_ICON_RESULT_READY = WM_APP + 1;
	static const UINT WM_APP_SUBFOLDERS_RESULT_READY = WM_APP + 2;

	/* Used to store the tree items as you enumerate them ready for sorting. */
	typedef struct
	{
		TCHAR		ItemName[MAX_PATH];
		int			iItemId;
	} ItemStore_t;

	typedef struct
	{
		TCHAR szFileName[MAX_PATH];
		DWORD dwAction;
	} AlteredFile_t;

	typedef struct
	{
		PIDLIST_ABSOLUTE pidl;
		PITEMID_CHILD pridl;
	} ItemInfo_t;

	struct BasicItemInfo
	{
		BasicItemInfo() = default;

		BasicItemInfo(const BasicItemInfo &other)
		{
			pidl.reset(ILCloneFull(other.pidl.get()));
		}

		unique_pidl_absolute pidl;
	};

	struct IconResult
	{
		HTREEITEM item;
		int iconIndex;
	};

	struct SubfoldersResult
	{
		HTREEITEM item;
		bool hasSubfolder;
	};

	typedef struct
	{
		TCHAR szPath[MAX_PATH];
		CMyTreeView *pMyTreeView;
	} DirectoryAltered_t;

	typedef struct
	{
		TCHAR	szDrive[MAX_PATH];
		HANDLE	hDrive;
		int		iMonitorId;
	} DriveEvent_t;

	/* Message handlers. */
	LRESULT CALLBACK	OnNotify(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void		AddDirectoryInternal(IShellFolder *pShellFolder, PCIDLIST_ABSOLUTE pidlDirectory, HTREEITEM hParent);
	void		DirectoryModified(DWORD dwAction, const TCHAR *szFullFileName);
	void		DirectoryAltered(void);
	HTREEITEM	AddRoot();
	void		AddItem(const TCHAR *szFullFileName);
	void		AddItemInternal(HTREEITEM hParent, const TCHAR *szFullFileName);
	void		AddDrive(const TCHAR *szDrive);
	void		RenameItem(HTREEITEM hItem, const TCHAR *szFullFileName);
	void		RemoveItem(const TCHAR *szFullFileName);
	void		RemoveItem(HTREEITEM hItem);
	void		UpdateParent(const TCHAR *szParent);
	void		UpdateParent(HTREEITEM hParent);
	LRESULT CALLBACK	OnDeviceChange(WPARAM wParam,LPARAM lParam);
	void		OnGetDisplayInfo(NMTVDISPINFO *pnmtvdi);
	void		UpdateChildren(HTREEITEM hParent, PCIDLIST_ABSOLUTE pidlParent);
	PCIDLIST_ABSOLUTE UpdateItemInfo(PCIDLIST_ABSOLUTE pidlParent, int iItemId);
	HTREEITEM	LocateDeletedItem(const TCHAR *szFullFileName);
	HTREEITEM	LocateItemByPath(const TCHAR *szItemPath, BOOL bExpand);
	HTREEITEM	LocateItemOnDesktopTree(const TCHAR *szFullFileName);

	static void	DirectoryAlteredCallback(const TCHAR *szFileName, DWORD dwAction, void *pData);

	/* Directory modification. */
	void		DirectoryAlteredAddFile(const TCHAR *szFullFileName);
	void		DirectoryAlteredRemoveFile(const TCHAR *szFullFileName);
	void		DirectoryAlteredRenameFile(const TCHAR *szFullFileName);

	/* Icons. */
	void		QueueIconTask(HTREEITEM item);
	static std::optional<IconResult>	FindIconAsync(HWND treeView, int iconResultId, HTREEITEM item, PCIDLIST_ABSOLUTE pidl);
	void		ProcessIconResult(int iconResultId);

	void		QueueSubfoldersTask(HTREEITEM item);
	static std::optional<SubfoldersResult> CheckSubfoldersAsync(HWND treeView, int subfoldersResultId, HTREEITEM item, PCIDLIST_ABSOLUTE pidl);
	void		ProcessSubfoldersResult(int subfoldersResultId);

	/* Item id's. */
	int			GenerateUniqueItemId(void);

	/* Drag and drop. */
	HRESULT		InitializeDragDropHelpers(void);
	void		RestoreState(void);
	DWORD		GetCurrentDragEffect(DWORD grfKeyState,DWORD dwCurrentEffect,POINTL *ptl);
	BOOL		CheckItemLocations(IDataObject *pDataObject,HTREEITEM hItem,int iDroppedItem);
	HRESULT		OnBeginDrag(int iItemId,DragTypes_t DragType);

	/* Icon refresh. */
	void		RefreshAllIconsInternal(HTREEITEM hFirstSibling);

	HTREEITEM	LocateExistingItem(const TCHAR *szParsingPath);
	HTREEITEM	LocateExistingItem(PCIDLIST_ABSOLUTE pidlDirectory);
	HTREEITEM	LocateItemInternal(PCIDLIST_ABSOLUTE pidlDirectory, BOOL bOnlyLocateExistingItem);
	void		MonitorAllDrives(void);
	void		MonitorDrive(const TCHAR *szDrive);
	HTREEITEM	DetermineDriveSortedPosition(HTREEITEM hParent, const TCHAR *szItemName);
	HTREEITEM	DetermineItemSortedPosition(HTREEITEM hParent, const TCHAR *szItem);
	BOOL		IsDesktop(const TCHAR *szPath);
	BOOL		IsDesktopSubChild(const TCHAR *szFullFileName);

	HWND				m_hTreeView;
	HWND				m_hParent;
	int					m_iRefCount;
	IDirectoryMonitor	*m_pDirMon;
	BOOL				m_bShowHidden;

	ctpl::thread_pool	m_iconThreadPool;
	std::unordered_map<int, std::future<std::optional<IconResult>>>	m_iconResults;
	int					m_iconResultIDCounter;

	ctpl::thread_pool	m_subfoldersThreadPool;
	std::unordered_map<int, std::future<std::optional<SubfoldersResult>>>	m_subfoldersResults;
	int					m_subfoldersResultIDCounter;

	/* Item id's and info. */
	int					*m_uItemMap;
	ItemInfo_t			*m_pItemInfo;
	int					m_iCurrentItemAllocation;
	int					m_iFolderIcon;

	/* Drag and drop. */
	BOOL				m_bDragDropRegistered;
	IDragSourceHelper	*m_pDragSourceHelper;
	IDropTargetHelper	*m_pDropTargetHelper;
	IDataObject			*m_pDataObject;
	BOOL				m_bDragging;
	BOOL				m_bDragCancelled;
	BOOL				m_bDragAllowed;
	BOOL				m_bDataAccept;
	DragTypes_t			m_DragType;

	/* Directory modification. */
	std::list<AlteredFile_t>	m_AlteredList;
	std::list<AlteredFile_t>	m_AlteredTrackingList;
	CRITICAL_SECTION	m_cs;
	TCHAR				m_szAlteredOldFileName[MAX_PATH];

	/* Hardware events. */
	std::list<DriveEvent_t>	m_pDriveList;
	BOOL				m_bQueryRemoveCompleted;
	TCHAR				m_szQueryRemove[MAX_PATH];
};