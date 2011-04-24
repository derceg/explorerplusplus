#ifndef TREEVIEW_INCLUDED
#define TREEVIEW_INCLUDED

#include <commctrl.h>
#include <shlwapi.h>
#include <list>
#include <vector>
#include "../Helper/iDirectoryMonitor.h"
#include "../Helper/DropHandler.h"

#define WM_USER_TREEVIEW				WM_APP + 70
#define WM_USER_TREEVIEW_GAINEDFOCUS	(WM_USER_TREEVIEW + 2)

class CMyTreeView : public IDropTarget, public IDropSource
{
public:

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

	CMyTreeView(HWND hTreeView,HWND hParent,IDirectoryMonitor *pDirMon,HANDLE hIconsThread);
	~CMyTreeView();

	/* Drop source functions. */
	HRESULT _stdcall	QueryContinueDrag(BOOL fEscapePressed,DWORD gfrKeyState);
	HRESULT _stdcall	GiveFeedback(DWORD dwEffect);

	/* User functions. */
	LRESULT CALLBACK	TreeViewProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
	HRESULT				AddDirectory(HTREEITEM hParent,TCHAR *szParsingPath);
	HRESULT				AddDirectory(HTREEITEM hParent,LPITEMIDLIST pidlDirectory);
	void				AddDirectoryInternal(IShellFolder *pShellFolder,LPITEMIDLIST pidlDirectory,HTREEITEM hParent);
	LPITEMIDLIST		BuildPath(HTREEITEM hTreeItem);
	HTREEITEM			LocateItem(TCHAR *szParsingPath);
	HTREEITEM			LocateItem(LPITEMIDLIST pidlDirectory);
	HTREEITEM			LocateDeletedItem(IN TCHAR *szFullFileName);
	HTREEITEM			LocateItemByPath(TCHAR *szItemPath,BOOL bExpand);
	void				EraseItems(HTREEITEM hParent);
	HTREEITEM			LocateItemOnDesktopTree(TCHAR *szFullFileName);
	BOOL				QueryDragging(void);
	DWORD WINAPI		Thread_SubFolders(LPVOID pParam);
	DWORD WINAPI		Thread_AddDirectoryInternal(IShellFolder *pShellFolder,LPITEMIDLIST pidlDirectory,HTREEITEM hParent);
	void				SetShowHidden(BOOL bShowHidden);
	void				RefreshAllIcons(void);

	/* Sorting. */
	int CALLBACK		CompareItems(LPARAM lParam1,LPARAM lParam2);

	static void DirectoryAlteredCallback(TCHAR *szFileName,DWORD dwAction,void *pData);

	/* Drag and Drop. */
	HRESULT _stdcall	DragEnter(IDataObject *pDataObject,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragLeave(void);
	HRESULT _stdcall	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);

	void				MonitorDrivePublic(TCHAR *szDrive);

	int					m_iProcessing;

private:

	/* Message handlers. */
	LRESULT CALLBACK	OnNotify(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);
	LRESULT		OnSetCursor(void);

	void		DirectoryModified(DWORD dwAction,TCHAR *szFullFileName);
	void		DirectoryAltered(void);
	HTREEITEM	AddRoot(void);
	void		AddItem(TCHAR *szFullFileName);
	void		AddItemInternal(HTREEITEM hParent,TCHAR *szFullFileName);
	void		AddDrive(TCHAR *szDrive);
	void		RenameItem(HTREEITEM hItem,TCHAR *szFullFileName);
	void		RemoveItem(TCHAR *szFullFileName);
	void		RemoveItem(HTREEITEM hItem);
	void		UpdateParent(TCHAR *szParent);
	void		UpdateParent(HTREEITEM hParent);
	LRESULT CALLBACK	OnDeviceChange(WPARAM wParam,LPARAM lParam);
	void		OnGetDisplayInfo(LPARAM lParam);
	void		UpdateChildren(HTREEITEM hParent,LPITEMIDLIST pidlParent);
	LPITEMIDLIST	UpdateItemInfo(LPITEMIDLIST pidlParent,int iItemId);

	/* Directory modification. */
	void		DirectoryAlteredAddFile(TCHAR *szFullFileName);
	void		DirectoryAlteredRemoveFile(TCHAR *szFullFileName);
	void		DirectoryAlteredRenameFile(TCHAR *szFullFileName);

	/* Icons. */
	void		AddToIconFinderQueue(TVITEM *plvItem);
	void		EmptyIconFinderQueue(void);

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

	HTREEITEM	LocateExistingItem(TCHAR *szParsingPath);
	HTREEITEM	LocateExistingItem(LPITEMIDLIST pidlDirectory);
	HTREEITEM	LocateItemInternal(LPITEMIDLIST pidlDirectory,BOOL bOnlyLocateExistingItem);
	void		MonitorAllDrives(void);
	void		MonitorDrive(TCHAR *szDrive);
	HTREEITEM	DetermineDriveSortedPosition(HTREEITEM hParent,TCHAR *szItemName);
	HTREEITEM	DetermineItemSortedPosition(HTREEITEM hParent,TCHAR *szItem);
	BOOL		IsDesktop(TCHAR *szPath);
	BOOL		IsDesktopSubChild(TCHAR *szFullFileName);




	/* ------ Internal state. ------ */

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
		LPITEMIDLIST	pidl;
		LPITEMIDLIST	pridl;
	} ItemInfo_t;

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

	HWND				m_hTreeView;
	HWND				m_hParent;
	int					m_iRefCount;
	IDirectoryMonitor	*m_pDirMon;
	TCHAR				m_szOldName[MAX_PATH];
	BOOL				m_bRightClick;
	BOOL				m_bShowHidden;

	/* Subfolder thread. */
	CRITICAL_SECTION	m_csSubFolders;

	/* Icon thread. */
	HANDLE				m_hThread;

	/* Item id's and info. */
	int					*m_uItemMap;
	ItemInfo_t			*m_pItemInfo;
	int					m_iCurrentItemAllocation;
	int					m_iFolderIcon;

	/* Drag and drop. */
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

typedef struct
{
	HWND			hTreeView;
	HTREEITEM		hParent;
	LPITEMIDLIST	pidl;
	CMyTreeView		*pMyTreeView;

	IShellFolder	*pShellFolder;
} ThreadInfo_t;

#endif