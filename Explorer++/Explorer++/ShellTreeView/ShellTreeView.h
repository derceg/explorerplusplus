// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/DropHandler.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowSubclassWrapper.h"
#include "../Helper/iDirectoryMonitor.h"
#include "../ThirdParty/CTPL/cpl_stl.h"
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <optional>

class CachedIcons;
struct Config;
class FileActionHandler;
__interface IExplorerplusplus;
class TabContainer;

class ShellTreeView : public IDropTarget, public IDropSource
{
public:
	/* IUnknown methods. */
	HRESULT __stdcall QueryInterface(REFIID iid, void **ppvObject) override;
	ULONG __stdcall AddRef() override;
	ULONG __stdcall Release() override;

	ShellTreeView(HWND hParent, IExplorerplusplus *coreInterface, IDirectoryMonitor *pDirMon,
		TabContainer *tabContainer, FileActionHandler *fileActionHandler, CachedIcons *cachedIcons);
	~ShellTreeView();

	/* Drop source functions. */
	HRESULT _stdcall QueryContinueDrag(BOOL fEscapePressed, DWORD gfrKeyState) override;
	HRESULT _stdcall GiveFeedback(DWORD dwEffect) override;

	/* User functions. */
	unique_pidl_absolute GetItemPidl(HTREEITEM hTreeItem) const;
	HTREEITEM LocateItem(PCIDLIST_ABSOLUTE pidlDirectory);
	BOOL QueryDragging();
	void SetShowHidden(BOOL bShowHidden);
	void RefreshAllIcons();

	/* Sorting. */
	int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2);

	/* Drag and Drop. */
	HRESULT _stdcall DragEnter(
		IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
	HRESULT _stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
	HRESULT _stdcall DragLeave() override;
	HRESULT _stdcall Drop(
		IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;

	void MonitorDrivePublic(const TCHAR *szDrive);

	HWND GetHWND() const;
	void StartRenamingSelectedItem();
	void ShowPropertiesOfSelectedItem() const;
	void DeleteSelectedItem(bool permanent);
	void CopySelectedItemToClipboard(bool copy);
	void PasteClipboardData();

private:
	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static const UINT WM_APP_ICON_RESULT_READY = WM_APP + 1;
	static const UINT WM_APP_SUBFOLDERS_RESULT_READY = WM_APP + 2;

	// This is the same background color as used in the Explorer treeview.
	static inline constexpr COLORREF TREE_VIEW_DARK_MODE_BACKGROUND_COLOR = RGB(25, 25, 25);

	typedef struct
	{
		unique_pidl_absolute pidl;
		unique_pidl_child pridl;
	} ItemInfo_t;

	typedef struct
	{
		int internalIndex;
		std::wstring name;
	} EnumeratedItem;

	typedef struct
	{
		TCHAR szFileName[MAX_PATH];
		DWORD dwAction;
	} AlteredFile_t;

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
		int internalIndex;
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
		ShellTreeView *shellTreeView;
	} DirectoryAltered_t;

	typedef struct
	{
		TCHAR szDrive[MAX_PATH];
		HANDLE hDrive;
		int iMonitorId;
	} DriveEvent_t;

	static HWND CreateTreeView(HWND parent);

	static LRESULT CALLBACK TreeViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK TreeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HRESULT ExpandDirectory(HTREEITEM hParent);
	void AddDirectoryInternal(
		IShellFolder *pShellFolder, PCIDLIST_ABSOLUTE pidlDirectory, HTREEITEM hParent);
	void DirectoryModified(DWORD dwAction, const TCHAR *szFullFileName);
	void DirectoryAltered();
	HTREEITEM AddRoot();
	void AddItem(const TCHAR *szFullFileName);
	void AddItemInternal(HTREEITEM hParent, const TCHAR *szFullFileName);
	void AddDrive(const TCHAR *szDrive);
	void RenameItem(HTREEITEM hItem, const TCHAR *szFullFileName);
	void RemoveItem(const TCHAR *szFullFileName);
	void RemoveItem(HTREEITEM hItem);
	void EraseItems(HTREEITEM hParent);
	void UpdateParent(const TCHAR *szParent);
	void UpdateParent(HTREEITEM hParent);
	LRESULT CALLBACK OnDeviceChange(WPARAM wParam, LPARAM lParam);
	void OnGetDisplayInfo(NMTVDISPINFO *pnmtvdi);
	void OnItemExpanding(const NMTREEVIEW *nmtv);
	LRESULT OnKeyDown(const NMTVKEYDOWN *keyDown);
	void UpdateChildren(HTREEITEM hParent, PCIDLIST_ABSOLUTE pidlParent);
	PCIDLIST_ABSOLUTE UpdateItemInfo(PCIDLIST_ABSOLUTE pidlParent, int iItemId);
	HTREEITEM LocateDeletedItem(const TCHAR *szFullFileName);
	HTREEITEM LocateItemByPath(const TCHAR *szItemPath, BOOL bExpand);
	HTREEITEM LocateItemOnDesktopTree(const TCHAR *szFullFileName);
	void OnMiddleButtonDown(const POINT *pt);
	void OnMiddleButtonUp(const POINT *pt);
	bool OnEndLabelEdit(const NMTVDISPINFO *dispInfo);

	void UpdateCurrentClipboardObject(wil::com_ptr<IDataObject> clipboardDataObject);
	void OnClipboardUpdate();

	static void DirectoryAlteredCallback(const TCHAR *szFileName, DWORD dwAction, void *pData);

	unique_pidl_absolute GetSelectedItemPidl() const;

	/* Directory modification. */
	void DirectoryAlteredAddFile(const TCHAR *szFullFileName);
	void DirectoryAlteredRemoveFile(const TCHAR *szFullFileName);
	void DirectoryAlteredRenameFile(const TCHAR *szFullFileName);

	/* Icons. */
	void QueueIconTask(HTREEITEM item, int internalIndex);
	static std::optional<IconResult> FindIconAsync(
		HWND treeView, int iconResultId, HTREEITEM item, int internalIndex, PCIDLIST_ABSOLUTE pidl);
	void ProcessIconResult(int iconResultId);
	std::optional<int> GetCachedIconIndex(const ItemInfo_t &itemInfo);

	void QueueSubfoldersTask(HTREEITEM item);
	static std::optional<SubfoldersResult> CheckSubfoldersAsync(
		HWND treeView, int subfoldersResultId, HTREEITEM item, PCIDLIST_ABSOLUTE pidl);
	void ProcessSubfoldersResult(int subfoldersResultId);

	/* Item id's. */
	int GenerateUniqueItemId();

	const ItemInfo_t &GetItemByHandle(HTREEITEM item) const;
	ItemInfo_t &GetItemByHandle(HTREEITEM item);
	int GetItemInternalIndex(HTREEITEM item) const;

	/* Drag and drop. */
	HRESULT InitializeDragDropHelpers();
	void RestoreState();
	DWORD GetCurrentDragEffect(DWORD grfKeyState, DWORD dwCurrentEffect, POINTL *ptl);
	BOOL CheckItemLocations(IDataObject *pDataObject, HTREEITEM hItem, int iDroppedItem);
	HRESULT OnBeginDrag(int iItemId, DragType dragType);

	/* Icon refresh. */
	void RefreshAllIconsInternal(HTREEITEM hFirstSibling);

	HTREEITEM LocateExistingItem(const TCHAR *szParsingPath);
	HTREEITEM LocateExistingItem(PCIDLIST_ABSOLUTE pidlDirectory);
	HTREEITEM LocateItemInternal(PCIDLIST_ABSOLUTE pidlDirectory, BOOL bOnlyLocateExistingItem);
	void MonitorDrive(const TCHAR *szDrive);
	HTREEITEM DetermineDriveSortedPosition(HTREEITEM hParent, const TCHAR *szItemName);
	HTREEITEM DetermineItemSortedPosition(HTREEITEM hParent, const TCHAR *szItem);
	BOOL IsDesktop(const TCHAR *szPath);
	BOOL IsDesktopSubChild(const TCHAR *szFullFileName);
	void UpdateItemState(HTREEITEM item, UINT stateMask, UINT state);

	void OnApplicationShuttingDown();

	HWND m_hTreeView;
	int m_iRefCount;
	IDirectoryMonitor *m_pDirMon;
	BOOL m_bShowHidden;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
	const Config *m_config;
	TabContainer *m_tabContainer;
	FileActionHandler *m_fileActionHandler;

	ctpl::thread_pool m_iconThreadPool;
	std::unordered_map<int, std::future<std::optional<IconResult>>> m_iconResults;
	int m_iconResultIDCounter;

	ctpl::thread_pool m_subfoldersThreadPool;
	std::unordered_map<int, std::future<std::optional<SubfoldersResult>>> m_subfoldersResults;
	int m_subfoldersResultIDCounter;

	/* Item id's and info. */
	std::unordered_map<int, ItemInfo_t> m_itemInfoMap;
	int m_itemIDCounter;
	CachedIcons *m_cachedIcons;

	int m_iFolderIcon;

	HTREEITEM m_middleButtonItem;

	/* Drag and drop. */
	BOOL m_bDragDropRegistered;
	IDragSourceHelper *m_pDragSourceHelper;
	IDropTargetHelper *m_pDropTargetHelper;
	IDataObject *m_pDataObject;
	BOOL m_bDragging;
	BOOL m_bDragCancelled;
	BOOL m_bDragAllowed;
	BOOL m_bDataAccept;
	DragType m_DragType;

	HTREEITEM m_cutItem;
	wil::com_ptr<IDataObject> m_clipboardDataObject;

	/* Directory modification. */
	std::list<AlteredFile_t> m_AlteredList;
	std::list<AlteredFile_t> m_AlteredTrackingList;
	CRITICAL_SECTION m_cs;
	TCHAR m_szAlteredOldFileName[MAX_PATH];

	/* Hardware events. */
	std::list<DriveEvent_t> m_pDriveList;
	BOOL m_bQueryRemoveCompleted;
	TCHAR m_szQueryRemove[MAX_PATH];
};