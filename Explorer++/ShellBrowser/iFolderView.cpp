/******************************************************************
 *
 * Project: ShellBrowser
 * File: iFolderView.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Constructs/deconstructs the browser
 * object. Also contains some auxillary
 * code.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <string>
#include "IShellView.h"
#include "iShellBrowser_internal.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/RegistrySettings.h"


BOOL g_bInitialized = FALSE;

/* IUnknown interface members. */
HRESULT __stdcall CFolderView::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IFolderView)
	{
		*ppvObject = static_cast<MyIFolderView2 *>(this);
	}
	else if(iid == IID_IShellView)
	{
		*ppvObject=static_cast<MyIShellView3 *>(this);
	}
	else if(iid == IID_IShellBrowser)
	{
		*ppvObject = static_cast<IShellBrowser2 *>(this);
	}
	else if(iid == IID_IShellFolder)
	{
		*ppvObject = static_cast <IShellFolder3 *> (this);
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CFolderView::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall CFolderView::Release(void)
{
	m_iRefCount--;
	
	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

void InitializeFolderView(HWND hOwner,HWND hListView,
MyIFolderView2 **pFolderView,InitialSettings_t *pSettings,
HANDLE hIconThread,HANDLE hFolderSizeThread)
{
	*pFolderView = new CFolderView(hOwner,hListView,pSettings,
		hIconThread,hFolderSizeThread);
}

/* CFolderView constructor. */
CFolderView::CFolderView(HWND hOwner,HWND hListView,
InitialSettings_t *pSettings,HANDLE hIconThread,
HANDLE hFolderSizeThread)
{
	HRESULT hr;

	m_iRefCount = 1;

	m_hListView	= hListView;
	m_hOwner	= hOwner;

	hr = InitializeDragDropHelpers();

	//if(!SUCCEEDED(hr));

	AllocateInitialItemMemory();

	OSVERSIONINFO VersionInfo;

	VersionInfo.dwOSVersionInfoSize	= sizeof(OSVERSIONINFO);

	/* Need the OS version (for grouping). */
	if(GetVersionEx(&VersionInfo) != 0)
	{
		m_dwMajorVersion = VersionInfo.dwMajorVersion;
	}

	m_pPathManager			= new CPathManager();

	m_SortMode				= FSM_NAME;
	m_ViewMode				= VM_ICONS;
	m_bSortAscending		= TRUE;
	m_bAutoArrange			= TRUE;
	m_bShowInGroups			= FALSE;
	m_bShowFriendlyDates	= TRUE;
	m_bFolderVisited		= FALSE;
	m_bApplyFilter			= FALSE;
	m_bFilterCaseSensitive	= FALSE;
	m_bGridlinesActive		= TRUE;
	m_bShowHidden			= FALSE;
	m_bShowExtensions		= TRUE;
	m_bHideSystemFiles		= FALSE;
	m_bHideLinkExtension	= FALSE;
	m_bHideRecycleBin		= FALSE;
	m_bHideSysVolInfo		= FALSE;

	/* Internal state. */
	m_bColumnsPlaced		= FALSE;
	m_bOverFolder			= FALSE;
	m_bDragging				= FALSE;
	m_bVirtualFolder		= FALSE;
	m_bThumbnailsSetup		= FALSE;
	m_nCurrentColumns		= 0;
	m_iDirMonitorId			= -1;
	m_iParentDirMonitorId	= -1;
	m_nItemsInInfoList		= 0;
	m_nInfoListAllocation	= 0;
	m_bFolderChanging		= FALSE;
	m_pActiveColumnList		= NULL;
	m_bPerformingDrag		= FALSE;
	m_bNotifiedOfTermination	= FALSE;
	m_nActiveColumns		= 0;
	m_bViewSet				= FALSE;
	m_bNewItemCreated		= FALSE;
	m_iDropped				= -1;

	m_iUniqueFolderIndex	= 0;

	m_pidlDirectory			= NULL;

	SetUserOptions(pSettings);

	ListView_SetAutoArrange(m_hListView,m_bAutoArrange);

	ListView_SetGridlines(m_hListView,m_bGridlinesActive);

	m_nAwaitingAdd = 0;

	m_pItemMap = (int *)malloc(m_iCurrentAllocation * sizeof(int));

	InitializeItemMap(0,m_iCurrentAllocation);

	InitializeCriticalSection(&m_csDirectoryAltered);
	InitializeCriticalSection(&m_column_cs);
	InitializeCriticalSection(&m_folder_cs);

	if(!g_bcsThumbnailInitialized)
	{
		InitializeCriticalSection(&g_csThumbnails);
		g_bcsThumbnailInitialized = TRUE;
	}

	m_bIconThreadSleeping = TRUE;
	m_hThread = hIconThread;
	m_hFolderSizeThread = hFolderSizeThread;

	m_iFolderIcon = GetDefaultFolderIconIndex();
	m_iFileIcon = GetDefaultFileIconIndex();

	m_nAPCsRan = 0;
	m_nAPCsQueued = 0;

	if(!g_bInitialized)
	{
		g_nAPCsRan = 0;
		g_nAPCsQueued = 0;
		g_bIconThreadSleeping = TRUE;
		InitializeCriticalSection(&g_icon_cs);
		g_nItemsInInfoList = 0;

		g_bInitialized = TRUE;
	}

	m_hIconEvent = CreateEvent(NULL,TRUE,TRUE,NULL);
	m_hColumnQueueEvent = CreateEvent(NULL,TRUE,TRUE,NULL);
	m_hFolderQueueEvent = CreateEvent(NULL,TRUE,TRUE,NULL);
}

/* CFolderView deconstructor. */
CFolderView::~CFolderView()
{
	EmptyIconFinderQueue();
	EmptyThumbnailsQueue();
	EmptyColumnQueue();
	EmptyFolderQueue();

	/* Wait for any current processing to finish. */
	WaitForSingleObject(m_hIconEvent,INFINITE);
	//WaitForSingleObject(m_hColumnQueueEvent,INFINITE);
	//WaitForSingleObject(m_hFolderQueueEvent,INFINITE);

	/* Release the drag and drop helpers. */
	m_pDropTargetHelper->Release();
	m_pDragSourceHelper->Release();

	DeleteCriticalSection(&m_folder_cs);
	DeleteCriticalSection(&m_column_cs);
	DeleteCriticalSection(&m_csDirectoryAltered);

	LVITEM lvItem;
	int nItems;
	int i = 0;

	nItems = ListView_GetItemCount(m_hListView);

	for(i = 0;i < nItems;i++)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;

		ListView_GetItem(m_hListView,&lvItem);

		CoTaskMemFree((LPVOID)m_pExtraItemInfo[lvItem.lParam].pridl);

		/* Also destroy the thumbnails imagelist... */
	}

	CoTaskMemFree(m_pidlDirectory);

	m_nItemsInInfoList = 0;
	m_nInfoListAllocation = 0;

	m_pPathManager->Release();
	free(m_pItemMap);
	free(m_pExtraItemInfo);
	free(m_pwfdFiles);
}

BOOL CFolderView::GetAutoArrange(void)
{
	if(m_bAutoArrange)
	{
		return TRUE;
	}

	return FALSE;
}

/* This function is only called on 'hard' view changes
(i.e. view changes resulting from user requests). It is
not called when a tab is first set up (in which case
the view mode still needs to be setup), or when entering
a folder. */
HRESULT CFolderView::SetCurrentViewMode(DWORD ViewMode)
{
	if(ViewMode == m_ViewMode)
		return S_FALSE;

	if(m_ViewMode == VM_THUMBNAILS && ViewMode != VM_THUMBNAILS)
		RemoveThumbnailsView();

	SetCurrentViewModeInternal(ViewMode);

	switch(ViewMode)
	{
		case VM_DETAILS:
			{
				int i = 0;

				for(i = 0;i < m_nTotalItems;i++)
					AddToColumnQueue(i);

				QueueUserAPC(SetAllColumnDataAPC,m_hThread,(ULONG_PTR)this);

				if(m_bShowFolderSizes)
					QueueUserAPC(SetAllFolderSizeColumnDataAPC,m_hFolderSizeThread,(ULONG_PTR)this);
			}
			break;

		case VM_TILES:
			SetTileViewInfo();
			break;
	}

	return S_OK;
}

/* Explicitly sets the view mode within in the listview.
This function also initializes any items needed to support
the current view mode. This MUST be done within this
function, as when a tab is first opened, the view settings
will need to be initialized. */
void CFolderView::SetCurrentViewModeInternal(DWORD ViewMode)
{
	DWORD dwStyle;

	switch(ViewMode)
	{
	case VM_EXTRALARGEICONS:
		{
			IImageList *pImageList = NULL;

			SHGetImageList(SHIL_JUMBO,IID_IImageList,(void **)&pImageList);
			ListView_SetImageList(m_hListView,(HIMAGELIST)pImageList,LVSIL_NORMAL);
			pImageList->Release();
		}
		break;

	case VM_LARGEICONS:
		{
			IImageList *pImageList = NULL;

			SHGetImageList(SHIL_EXTRALARGE,IID_IImageList,(void **)&pImageList);
			ListView_SetImageList(m_hListView,(HIMAGELIST)pImageList,LVSIL_NORMAL);
			pImageList->Release();
		}
		break;

	/* Do nothing. This will setup the listview by itself. */
	case VM_THUMBNAILS:
		break;

	case VM_TILES:
	case VM_ICONS:
	case VM_SMALLICONS:
	case VM_LIST:
	case VM_DETAILS:
		{
			IImageList *pImageList = NULL;

			SHGetImageList(SHIL_LARGE,IID_IImageList,(void **)&pImageList);
			ListView_SetImageList(m_hListView,(HIMAGELIST)pImageList,LVSIL_NORMAL);
			pImageList->Release();
		}
		break;
	}

	/* Delete all the tile view columns. */
	if(m_ViewMode == VM_TILES && ViewMode != VM_TILES)
		DeleteTileViewColumns();

	switch(ViewMode)
	{
		case VM_TILES:
			dwStyle = LV_VIEW_TILE;

			InsertTileViewColumns();
			break;

		case VM_EXTRALARGEICONS:
		case VM_LARGEICONS:
		case VM_ICONS:
			dwStyle = LV_VIEW_ICON;
			break;

		case VM_SMALLICONS:
			dwStyle = LV_VIEW_SMALLICON;
			break;

		case VM_LIST:
			dwStyle = LV_VIEW_LIST;
			break;

		case VM_DETAILS:
			dwStyle = LV_VIEW_DETAILS;

			if(!m_bColumnsPlaced)
			{
				PlaceColumns();
				m_bColumnsPlaced = TRUE;
			}
			break;

		case VM_THUMBNAILS:
			dwStyle = LV_VIEW_ICON;

			if(!m_bThumbnailsSetup)
				SetupThumbnailsView();
			break;

		default:
			dwStyle = LV_VIEW_ICON;
			ViewMode = VM_ICONS;
			break;
	}

	m_ViewMode = ViewMode;

	if(ViewMode != VM_DETAILS)
		ListView_SetGridlines(m_hListView,FALSE);
	else
		ListView_SetGridlines(m_hListView,m_bGridlinesActive);

	SendMessage(m_hListView,LVM_SETVIEW,dwStyle,0);
}

HRESULT CFolderView::GetCurrentViewMode(UINT *pViewMode)
{
	if(pViewMode == NULL)
		return E_INVALIDARG;

	*pViewMode = m_ViewMode;

	return S_OK;
}

HRESULT CFolderView::GetSortMode(UINT *SortMode)
{
	*SortMode = m_SortMode;

	return S_OK;
}

HRESULT CFolderView::SetSortMode(UINT SortMode)
{
	m_SortMode	= SortMode;

	return S_OK;
}

BOOL CFolderView::IsGroupViewEnabled(void)
{
	return m_bShowInGroups;
}

void CFolderView::InitializeItemMap(int iStart,int iEnd)
{
	int i = 0;

	for(i = iStart;i < iEnd;i++)
	{
		m_pItemMap[i] = 0;
	}

	m_iCachedPosition = 0;
}

HRESULT CFolderView::InitializeDragDropHelpers(void)
{
	HRESULT hr;

	/* Initialize the drag source helper, and use it to initialize the drop target helper. */
	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_INPROC_SERVER,
	IID_IDragSourceHelper,(LPVOID *)&m_pDragSourceHelper);

	if(SUCCEEDED(hr))
	{
		hr = m_pDragSourceHelper->QueryInterface(IID_IDropTargetHelper,(LPVOID *)&m_pDropTargetHelper);

		RegisterDragDrop(m_hListView,this);

		/* RegisterDragDrop calls AddRef on initialization. */
		Release();
	}

	return hr;
}

InitialSettings_t *CFolderView::QueryUserOptions(void)
{
	static InitialSettings_t is;

	/* TODO: Fix up. */
	is.bAutoArrange			= m_bAutoArrange;
	is.bGridlinesActive		= m_bGridlinesActive;
	is.bShowHidden			= m_bShowHidden;
	is.bShowInGroups		= m_bShowInGroups;
	is.bSortAscending		= m_bSortAscending;
	is.SortMode				= m_SortMode;
	is.ViewMode				= m_ViewMode;

	return &is;
}

void CFolderView::SetUserOptions(InitialSettings_t *is)
{
	m_bAutoArrange			= is->bAutoArrange;
	m_bGridlinesActive		= is->bGridlinesActive;
	m_bShowHidden			= is->bShowHidden;
	m_bShowInGroups			= is->bShowInGroups;
	m_bSortAscending		= is->bSortAscending;
	m_SortMode				= is->SortMode;
	m_ViewMode				= is->ViewMode;
	m_bApplyFilter			= is->bApplyFilter;
	m_bFilterCaseSensitive	= is->bFilterCaseSensitive;
	m_bShowFolderSizes		= is->bShowFolderSizes;
	m_bDisableFolderSizesNetworkRemovable = is->bDisableFolderSizesNetworkRemovable;
	m_bHideSystemFiles		= is->bHideSystemFiles;
	m_bHideLinkExtension	= is->bHideLinkExtension;
	m_bForceSize			= is->bForceSize;
	m_SizeDisplayFormat		= is->sdf;
	m_bHideRecycleBin		= is->bHideRecycleBin;
	m_bHideSysVolInfo		= is->bHideSysVolInfo;

	StringCchCopy(m_szFilter,SIZEOF_ARRAY(m_szFilter),is->szFilter);

	CopyColumnsInternal(&m_ControlPanelColumnList,is->pControlPanelColumnList);
	CopyColumnsInternal(&m_MyComputerColumnList,is->pMyComputerColumnList);
	CopyColumnsInternal(&m_MyNetworkPlacesColumnList,is->pMyNetworkPlacesColumnList);
	CopyColumnsInternal(&m_NetworkConnectionsColumnList,is->pNetworkConnectionsColumnList);
	CopyColumnsInternal(&m_PrintersColumnList,is->pPrintersColumnList);
	CopyColumnsInternal(&m_RealFolderColumnList,is->pRealFolderColumnList);
	CopyColumnsInternal(&m_RecycleBinColumnList,is->pRecycleBinColumnList);

	ListView_SetGridlines(m_hListView,m_bGridlinesActive);
}

void CFolderView::CopyColumnsInternal(list<Column_t> *pInternalColumns,list<Column_t> *pColumns)
{
	list<Column_t>::iterator	itr;
	Column_t			ci;

	pInternalColumns->clear();

	for(itr = pColumns->begin();itr != pColumns->end();itr++)
	{
		ci.id		= itr->id;
		ci.bChecked	= itr->bChecked;
		ci.iWidth	= itr->iWidth;

		pInternalColumns->push_back(ci);
	}
}

void CFolderView::SetGlobalSettings(GlobalSettings_t *gs)
{
	m_bShowExtensions		= gs->bShowExtensions;
	m_bShowFriendlyDates	= gs->bShowFriendlyDates;
	m_bShowFolderSizes		= gs->bShowFolderSizes;
}

int CFolderView::GetId(void)
{
	return m_ID;
}

void CFolderView::SetId(int ID)
{
	m_ID = ID;
}

void CFolderView::AllocateInitialItemMemory(void)
{
	m_pwfdFiles			= (WIN32_FIND_DATA *)malloc(DEFAULT_MEM_ALLOC * sizeof(WIN32_FIND_DATA));
	m_pExtraItemInfo	= (CItemObject *)malloc(DEFAULT_MEM_ALLOC * sizeof(CItemObject));

	m_iCurrentAllocation = DEFAULT_MEM_ALLOC;

	if((m_pwfdFiles == NULL) || (m_pExtraItemInfo == NULL))
	{
		return;
	}
}

void CFolderView::ToggleGridlines(void)
{
	m_bGridlinesActive = !m_bGridlinesActive;

	ListView_SetGridlines(m_hListView,m_bGridlinesActive);
}

BOOL CFolderView::QueryGridlinesActive(void)
{
	return m_bGridlinesActive;
}

void CFolderView::SetResourceModule(HINSTANCE hResourceModule)
{
	m_hResourceModule = hResourceModule;
}

void CFolderView::Terminate(void)
{
	SendMessage(m_hOwner,WM_USER_RELEASEBROWSER,(WPARAM)m_ID,NULL);
}

int CFolderView::GetNumAPCsRun(void)
{
	return g_nAPCsRan;
}

int CFolderView::GetNumAPCsQueued(void)
{
	return g_nAPCsQueued;
}

void CFolderView::IncrementNumAPCsRan(void)
{
	m_nAPCsRan++;
	g_nAPCsRan++;
}

HRESULT CFolderView::Refresh()
{
	BrowseFolder(m_pidlDirectory,SBSP_SAMEBROWSER|SBSP_ABSOLUTE|SBSP_WRITENOHISTORY);

	return S_OK;
}

void CFolderView::SetHideSystemFiles(BOOL bHideSystemFiles)
{
	m_bHideSystemFiles = bHideSystemFiles;
}

BOOL CFolderView::GetHideSystemFiles(void)
{
	return m_bHideSystemFiles;
}

void CFolderView::SetShowExtensions(BOOL bShowExtensions)
{
	m_bShowExtensions = bShowExtensions;
}

BOOL CFolderView::GetShowExtensions(void)
{
	return m_bShowExtensions;
}

void CFolderView::SetHideLinkExtension(BOOL bHideLinkExtension)
{
	m_bHideLinkExtension = bHideLinkExtension;
}

BOOL CFolderView::GetHideLinkExtension(void)
{
	return m_bHideLinkExtension;
}

void CFolderView::SetShowFolderSizes(BOOL bShowFolderSizes)
{
	m_bShowFolderSizes = bShowFolderSizes;
}

BOOL CFolderView::GetShowFolderSizes(void)
{
	return m_bShowFolderSizes;
}

void CFolderView::SetDisableFolderSizesNetworkRemovable(BOOL bDisableFolderSizesNetworkRemovable)
{
	m_bDisableFolderSizesNetworkRemovable = bDisableFolderSizesNetworkRemovable;
}

void CFolderView::SetShowFriendlyDates(BOOL bShowFriendlyDates)
{
	m_bShowFriendlyDates = bShowFriendlyDates;
}

BOOL CFolderView::GetShowFriendlyDates(void)
{
	return m_bShowFriendlyDates;
}

void CFolderView::SetInsertSorted(BOOL bInsertSorted)
{
	m_bInsertSorted = bInsertSorted;
}

BOOL CFolderView::GetInsertSorted(void)
{
	return m_bInsertSorted;
}

void CFolderView::SetHideRecycleBin(BOOL bHideRecycleBin)
{
	m_bHideRecycleBin = bHideRecycleBin;
}

BOOL CFolderView::GetHideRecycleBin(void)
{
	return m_bHideRecycleBin;
}

void CFolderView::SetHideSysVolInfo(BOOL bHideSysVolInfo)
{
	m_bHideSysVolInfo = bHideSysVolInfo;
}

BOOL CFolderView::GetHideSysVolInfo(void)
{
	return m_bHideSysVolInfo;
}

void CFolderView::SetForceSize(BOOL bForceSize)
{
	m_bForceSize = bForceSize;
}

void CFolderView::SetSizeDisplayFormat(SizeDisplayFormat_t sdf)
{
	m_SizeDisplayFormat = sdf;
}

void CFolderView::InsertTileViewColumns(void)
{
	LVTILEVIEWINFO lvtvi;
	LVCOLUMN lvColumn;

	/* Name. */
	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= EMPTY_STRING;
	ListView_InsertColumn(m_hListView,1,&lvColumn);

	/* Type. */
	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= EMPTY_STRING;
	ListView_InsertColumn(m_hListView,2,&lvColumn);

	/* File size. */
	lvColumn.mask		= LVCF_TEXT;
	lvColumn.pszText	= EMPTY_STRING;
	ListView_InsertColumn(m_hListView,3,&lvColumn);

	lvtvi.cbSize	= sizeof(lvtvi);
	lvtvi.dwMask	= LVTVIM_COLUMNS;
	lvtvi.dwFlags	= LVTVIF_AUTOSIZE;
	lvtvi.cLines	= 2;
	ListView_SetTileViewInfo(m_hListView,&lvtvi);
}

void CFolderView::DeleteTileViewColumns(void)
{
	ListView_DeleteColumn(m_hListView,3);
	ListView_DeleteColumn(m_hListView,2);
	ListView_DeleteColumn(m_hListView,1);
}

void CFolderView::SetTileViewInfo(void)
{
	LVITEM lvItem;
	BOOL bRes;
	int nItems;
	int i = 0;

	nItems = ListView_GetItemCount(m_hListView);

	for(i = 0;i < nItems;i++)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		bRes = ListView_GetItem(m_hListView,&lvItem);

		if(bRes)
		{
			SetTileViewItemInfo(i,(int)lvItem.lParam);
		}
	}
}

/* TODO: Make this function configurable. */
void CFolderView::SetTileViewItemInfo(int iItem,int iItemInternal)
{
	SHFILEINFO shfi;
	LVTILEINFO lvti;
	UINT uColumns[2] = {1,2};
	TCHAR FullFileName[MAX_PATH];

	lvti.cbSize		= sizeof(lvti);
	lvti.iItem		= iItem;
	lvti.cColumns	= 2;
	lvti.puColumns	= uColumns;
	ListView_SetTileInfo(m_hListView,&lvti);

	QueryFullItemName(iItem,FullFileName);

	SHGetFileInfo(FullFileName,0,
		&shfi,sizeof(SHFILEINFO),SHGFI_TYPENAME);

	ListView_SetItemText(m_hListView,iItem,1,shfi.szTypeName);

	if((m_pwfdFiles[iItemInternal].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
		FILE_ATTRIBUTE_DIRECTORY)
	{
		TCHAR			lpszFileSize[32];
		ULARGE_INTEGER	lFileSize;

		lFileSize.LowPart = m_pwfdFiles[iItemInternal].nFileSizeLow;
		lFileSize.HighPart = m_pwfdFiles[iItemInternal].nFileSizeHigh;

		FormatSizeString(lFileSize,lpszFileSize,SIZEOF_ARRAY(lpszFileSize),
			m_bForceSize,m_SizeDisplayFormat);

		ListView_SetItemText(m_hListView,iItem,2,lpszFileSize);
	}
}