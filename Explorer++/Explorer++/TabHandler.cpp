/******************************************************************
 *
 * Project: Explorer++
 * File: TabHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides tab management as well as the
 * handling of messages associated with the tabs.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Misc.h"
#include "Explorer++.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Bookmark.h"
#include "MainResource.h"


DWORD ListViewStyles		=	WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
								LVS_ICON|LVS_EDITLABELS|LVS_SHOWSELALWAYS|LVS_SHAREIMAGELISTS|
								LVS_AUTOARRANGE|WS_TABSTOP|LVS_ALIGNTOP;

UINT TabCtrlStyles			=	WS_VISIBLE|WS_CHILD|TCS_FOCUSNEVER|TCS_SINGLELINE|
								TCS_TOOLTIPS|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;

extern LRESULT CALLBACK	ListViewSubclassProcStub(HWND ListView,UINT msg,WPARAM wParam,LPARAM lParam);
extern LRESULT	(CALLBACK *DefaultListViewProc)(HWND,UINT,WPARAM,LPARAM);

void CContainer::InitializeTabMap(void)
{
	int i = 0;

	for(i = 0;i < MAX_TABS;i++)
	{
		m_uTabMap[i] = 0;
	}
}

void CContainer::ReleaseTabId(int iTabId)
{
	m_uTabMap[iTabId] = 0;
}

BOOL CContainer::CheckTabIdStatus(int iTabId)
{
	if(m_uTabMap[iTabId] == 0)
		return FALSE;

	return TRUE;
}

int CContainer::GenerateUniqueTabId(void)
{
	BOOL	bFound = FALSE;
	int		i = 0;

	for(i = 0;i < MAX_TABS;i++)
	{
		if(m_uTabMap[i] == 0)
		{
			m_uTabMap[i] = 1;
			bFound = TRUE;
			break;
		}
	}

	if(bFound)
		return i;
	else
		return -1;
}

HRESULT CContainer::CreateNewTab(TCHAR *TabDirectory,
InitialSettings_t *pSettings,TabInfo_t *pTabInfo,BOOL bSwitchToNewTab,
int *pTabObjectIndex)
{
	LPITEMIDLIST	pidl = NULL;
	TCHAR			szExpandedPath[MAX_PATH];
	HRESULT			hr;
	BOOL			bRet;

	/* Attempt to expand the path (in the event that
	it contains embedded environment variables). */
	bRet = MyExpandEnvironmentStrings(TabDirectory,
		szExpandedPath,SIZEOF_ARRAY(szExpandedPath));

	if(!bRet)
	{
		StringCchCopy(szExpandedPath,
			SIZEOF_ARRAY(szExpandedPath),TabDirectory);
	}

	if(!SUCCEEDED(GetIdlFromParsingName(szExpandedPath,&pidl)))
		return E_FAIL;

	hr = CreateNewTab(pidl,pSettings,pTabInfo,bSwitchToNewTab,pTabObjectIndex);

	CoTaskMemFree(pidl);

	return hr;
}

/* Creates a new tab. If the settings argument is NULL,
the global settings will be used. */
HRESULT CContainer::CreateNewTab(LPITEMIDLIST pidlDirectory,
InitialSettings_t *pSettings,TabInfo_t *pTabInfo,BOOL bSwitchToNewTab,
int *pTabObjectIndex)
{
	UINT				uFlags;
	HRESULT				hr;
	InitialSettings_t	is;
	int					iNewTabIndex;
	int					iTabId;

	if(!CheckIdl(pidlDirectory) || !IsIdlDirectory(pidlDirectory))
		return E_FAIL;

	if(m_bOpenNewTabNextToCurrent)
		iNewTabIndex = m_iTabSelectedItem + 1;
	else
		iNewTabIndex = TabCtrl_GetItemCount(m_hTabCtrl);

	iTabId = GenerateUniqueTabId();

	if(iTabId == -1)
		return E_FAIL;

	if(pTabInfo == NULL)
	{
		m_TabInfo[iTabId].bLocked			= FALSE;
		m_TabInfo[iTabId].bAddressLocked	= FALSE;
		m_TabInfo[iTabId].bUseCustomName	= FALSE;
	}
	else
	{
		m_TabInfo[iTabId] = *pTabInfo;
	}

	m_hListView[iTabId]	= CreateAndSubclassListView(m_hContainer,ListViewStyles);

	if(m_hListView[iTabId] == NULL)
		return E_FAIL;

	ListView_ActivateOneClickSelect(m_hListView[iTabId],m_bOneClickActivate);

	/* Set the listview to its initial size. */
	SetListViewInitialPosition(m_hListView[iTabId]);

	/* If no explicit settings are specified, use the
	global ones. */
	if(pSettings == NULL)
	{
		is.bAutoArrange			= m_bAutoArrangeGlobal;
		is.bGridlinesActive		= m_bShowGridlinesGlobal;
		is.bShowHidden			= m_bShowHiddenGlobal;
		is.bShowInGroups		= m_bShowInGroupsGlobal;
		is.bSortAscending		= m_bSortAscendingGlobal;
		is.SortMode				= DEFAULT_SORT_MODE;
		is.ViewMode				= m_ViewModeGlobal;
		is.bApplyFilter			= FALSE;
		is.bShowFolderSizes		= m_bShowFolderSizes;
		is.bDisableFolderSizesNetworkRemovable = m_bDisableFolderSizesNetworkRemovable;
		is.bShowSizeInBytes		= m_bShowSizesInBytesGlobal;
		is.bHideSystemFiles		= m_bHideSystemFilesGlobal;
		is.bHideLinkExtension	= m_bHideLinkExtensionGlobal;

		StringCchCopy(is.szFilter,SIZEOF_ARRAY(is.szFilter),EMPTY_STRING);

		is.pControlPanelColumnList			= &m_ControlPanelColumnList;
		is.pMyComputerColumnList			= &m_MyComputerColumnList;
		is.pMyNetworkPlacesColumnList		= &m_MyNetworkPlacesColumnList;
		is.pNetworkConnectionsColumnList	= &m_NetworkConnectionsColumnList;
		is.pPrintersColumnList				= &m_PrintersColumnList;
		is.pRealFolderColumnList			= &m_RealFolderColumnList;
		is.pRecycleBinColumnList			= &m_RecycleBinColumnList;

		pSettings = &is;
	}

	pSettings->bShowSizeInBytes = m_bShowSizesInBytesGlobal;

	InitializeFolderView(m_hContainer,m_hListView[iTabId],
	&m_pFolderView[iTabId],pSettings,m_hIconThread,m_hFolderSizeThread);

	if(pSettings->bApplyFilter)
		ListView_SetBackgroundImage(m_hListView[iTabId],IDB_FILTERINGAPPLIED);

	ListViewInfo_t	*plvi = NULL;

	plvi = (ListViewInfo_t *)malloc(sizeof(ListViewInfo_t));

	plvi->pContainer	= this;
	plvi->iObjectIndex	= iTabId;

	SetWindowLongPtr(m_hListView[iTabId],GWLP_USERDATA,(LONG_PTR)plvi);

	/* Subclass the window. */
	DefaultListViewProc = (WNDPROC)SetWindowLongPtr(m_hListView[iTabId],GWLP_WNDPROC,(LONG_PTR)ListViewSubclassProcStub);

	m_pFolderView[iTabId]->QueryInterface(IID_IShellBrowser,
	(void **)&m_pShellBrowser[iTabId]);

	m_pFolderView[iTabId]->SetId(iTabId);
	m_pFolderView[iTabId]->SetResourceModule(g_hLanguageModule);

	m_pShellBrowser[iTabId]->SetHideSystemFiles(m_bHideSystemFilesGlobal);
	m_pShellBrowser[iTabId]->SetShowExtensions(m_bShowExtensionsGlobal);
	m_pShellBrowser[iTabId]->SetHideLinkExtension(m_bHideLinkExtensionGlobal);
	m_pShellBrowser[iTabId]->SetShowFolderSizes(m_bShowFolderSizes);
	m_pShellBrowser[iTabId]->SetShowInBytes(m_bShowSizesInBytesGlobal);
	m_pShellBrowser[iTabId]->SetShowFriendlyDates(m_bShowFriendlyDatesGlobal);
	m_pShellBrowser[iTabId]->SetInsertSorted(m_bInsertSorted);

	/* Browse folder sends a message back to the main window, which
	attempts to contact the new tab (needs to be created before browsing
	the folder). */
	InsertNewTab(pidlDirectory,iNewTabIndex,iTabId);

	if(bSwitchToNewTab)
	{
		/* Select the newly created tab. */
		TabCtrl_SetCurSel(m_hTabCtrl,iNewTabIndex);

		/* Hide the previously active tab, and show the
		newly created one. */
		ShowWindow(m_hActiveListView,SW_HIDE);
		ShowWindow(m_hListView[iTabId],SW_SHOW);

		m_iObjectIndex			= iTabId;
		m_iTabSelectedItem		= iNewTabIndex;

		m_hActiveListView		= m_hListView[m_iObjectIndex];
		m_pActiveShellBrowser	= m_pShellBrowser[m_iObjectIndex];

		SetFocus(m_hListView[iTabId]);
	}

	/* SBSP_SAMEBROWSER is used internally. Ignored
	by the shellbrowser. */
	uFlags = SBSP_ABSOLUTE;

	/* These settings are applied to all tabs (i.e. they
	are not tab specific). Send them to the browser
	regardless of whether it loads its own settings or not. */
	PushGlobalSettingsToTab(iTabId);

	hr = m_pShellBrowser[iTabId]->BrowseFolder(pidlDirectory,uFlags);

	if(bSwitchToNewTab)
		m_pShellBrowser[iTabId]->QueryCurrentDirectory(MAX_PATH,m_CurrentDirectory);

	if(hr != S_OK)
	{
		/* Folder was not browsed. Likely that the path does not exist
		(or is locked, cannot be found, etc). */
		return E_FAIL;
	}

	if(pTabObjectIndex != NULL)
		*pTabObjectIndex = iTabId;

	return S_OK;
}

void CContainer::OnTabChangeInternal(void)
{
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,m_iTabSelectedItem,&tcItem);

	/* Hide the old listview. */
	ShowWindow(m_hActiveListView,SW_HIDE);

	m_iObjectIndex = (int)tcItem.lParam;

	m_hActiveListView		= m_hListView[m_iObjectIndex];
	m_pActiveShellBrowser	= m_pShellBrowser[m_iObjectIndex];

	m_nSelected = m_pActiveShellBrowser->QueryNumSelected();

	SetActiveArrangeMenuItems();
	UpdateArrangeMenuItems();

	UpdateWindowStates();

	/* Show the new listview. */
	ShowWindow(m_hActiveListView,SW_SHOW);

	SetFocus(m_hActiveListView);
}

void CContainer::RefreshAllTabs(void)
{
	int i = 0;
	int NumTabs;
	TCITEM tcItem;
	int iIndex;

	NumTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	for(i = 0;i < NumTabs;i++)
	{
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);
		iIndex = (int)tcItem.lParam;

		RefreshTab(iIndex);
	}
}

void CContainer::CloseOtherTabs(int iTab)
{
	int nTabs;
	int i = 0;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	/* Close all tabs except the
	specified one. */
	for(i = nTabs - 1;i >= 0; i--)
	{
		if(i != iTab)
		{
			CloseTab(i);
		}
	}
}

void CContainer::SelectAdjacentTab(BOOL bNextTab)
{
	int nTabs;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if(bNextTab)
	{
		/* If this is the last tab in the order,
		wrap the selection back to the start. */
		if(m_iTabSelectedItem == (nTabs - 1))
			m_iTabSelectedItem = 0;
		else
			m_iTabSelectedItem++;
	}
	else
	{
		/* If this is the first tab in the order,
		wrap the selection back to the end. */
		if(m_iTabSelectedItem == 0)
			m_iTabSelectedItem = nTabs - 1;
		else
			m_iTabSelectedItem--;
	}

	TabCtrl_SetCurSel(m_hTabCtrl,m_iTabSelectedItem);

	OnTabChangeInternal();
}

void CContainer::OnSelectTab(int iTab)
{
	int nTabs;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if(iTab == -1)
	{
		m_iTabSelectedItem = nTabs - 1;
	}
	else
	{
		if(iTab < nTabs)
			m_iTabSelectedItem = iTab;
		else
			m_iTabSelectedItem = nTabs - 1;
	}

	TabCtrl_SetCurSel(m_hTabCtrl,m_iTabSelectedItem);

	OnTabChangeInternal();
}

HRESULT CContainer::OnCloseTab(void)
{
	int iCurrentTab;

	iCurrentTab = TabCtrl_GetCurSel(m_hTabCtrl);

	return CloseTab(iCurrentTab);
}

HRESULT CContainer::CloseTab(int TabIndex)
{
	TCITEM	tcItem;
	int		NumTabs;
	int		m_iLastSelectedTab;
	int		ListViewIndex;
	int		iRemoveImage;

	m_iLastSelectedTab = TabIndex;

	NumTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if(NumTabs == 1)
	{
		/* Should never end up here. */
		return E_UNEXPECTED;
	}

	tcItem.mask = TCIF_IMAGE|TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,TabIndex,&tcItem);
	iRemoveImage = tcItem.iImage;

	/* The tab is locked. Don't close it. */
	if(m_TabInfo[(int)tcItem.lParam].bLocked || m_TabInfo[(int)tcItem.lParam].bAddressLocked)
		return S_FALSE;

	ListViewIndex = (int)tcItem.lParam;

	EnterCriticalSection(&g_csDirMonCallback);
	ReleaseTabId(ListViewIndex);
	LeaveCriticalSection(&g_csDirMonCallback);

	NumTabs--;

	/*
	Cases:
	If the tab been closed is the active tab:
	 - If the first tab is been closed, then the selected
	   tab will still be the first tab.
     - If the last tab is closed, then the selected tab
	   will be one less then the index of the previously
	   selected tab.
     - Otherwise, the index of the selected tab will remain
	   unchanged (as a tab will be pushed down).
   If the tab been closed is not the active tab:
     - If the index of the closed tab is less than the index
	   of the active tab, the index of the active tab will
	   decrease by one (as all higher tabs are pushed down
	   one space).
	*/
	if(TabIndex == m_iTabSelectedItem)
	{
		if(TabIndex == NumTabs)
		{
			m_iTabSelectedItem--;
			TabCtrl_SetCurSel(m_hTabCtrl,m_iTabSelectedItem);
			TabCtrl_DeleteItem(m_hTabCtrl,TabIndex);
		}
		else
		{
			TabCtrl_DeleteItem(m_hTabCtrl,TabIndex);
			TabCtrl_SetCurSel(m_hTabCtrl,m_iTabSelectedItem);
		}

		OnTabChangeInternal();
	}
	else
	{
		TabCtrl_DeleteItem(m_hTabCtrl,TabIndex);

		if(TabIndex < m_iTabSelectedItem)
			m_iTabSelectedItem--;
	}

	/* Remove the tabs image from the image list. */
	TabCtrl_RemoveImage(m_hTabCtrl,iRemoveImage);

	m_pDirMon->StopDirectoryMonitor(m_pShellBrowser[ListViewIndex]->GetDirMonitorId());

	m_pFolderView[ListViewIndex]->SetTerminationStatus();
	DestroyWindow(m_hListView[ListViewIndex]);

	m_pShellBrowser[ListViewIndex]->Release();
	m_pShellBrowser[ListViewIndex] = NULL;
	m_pFolderView[ListViewIndex]->Release();
	m_pFolderView[ListViewIndex] = NULL;

	HandleTabToolbarItemStates();

	return S_OK;
}

void CContainer::RefreshTab(int iTab)
{
	LPITEMIDLIST pidlDirectory = NULL;

	pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	/* Refresh the browser window (also updates all
	other associated child windows). */
	BrowseFolder(pidlDirectory,SBSP_SAMEBROWSER|SBSP_ABSOLUTE|
		SBSP_WRITENOHISTORY);

	CoTaskMemFree(pidlDirectory);
}

void CContainer::OnTabSelectionChange(void)
{
	m_iTabSelectedItem = TabCtrl_GetCurSel(m_hTabCtrl);

	OnTabChangeInternal();
}

LRESULT CALLBACK TabSubclassProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CContainer *pContainer = (CContainer *)dwRefData;

	return pContainer->TabSubclassProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CContainer::TabSubclassProc(HWND hTab,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITMENU:
			OnInitTabMenu(wParam);
			SendMessage(m_hContainer,WM_INITMENU,wParam,lParam);
			break;

		case WM_MENUSELECT:
			/* Forward the message to the main window so it can
			handle menu help. */
			SendMessage(m_hContainer,WM_MENUSELECT,wParam,lParam);
			break;

		case WM_MEASUREITEM:
			SendMessage(m_hContainer,WM_MEASUREITEM,wParam,lParam);
			break;

		case WM_DRAWITEM:
			SendMessage(m_hContainer,WM_DRAWITEM,wParam,lParam);
			break;

		case WM_LBUTTONDOWN:
			OnTabCtrlLButtonDown(wParam,lParam);
			break;

		case WM_LBUTTONUP:
			OnTabCtrlLButtonUp();
			break;

		case WM_MOUSEMOVE:
			OnTabCtrlMouseMove(wParam,lParam);
			break;

		case WM_MBUTTONUP:
			SendMessage(m_hContainer,WM_USER_TABMCLICK,wParam,lParam);
			break;

		case WM_RBUTTONUP:
			OnTabCtrlRButtonUp(wParam,lParam);
			break;

		case WM_CAPTURECHANGED:
			{
				if((HWND)lParam != hTab)
					ReleaseCapture();

				m_bTabBeenDragged = FALSE;
			}
			break;

		case WM_LBUTTONDBLCLK:
			{
				TCHITTESTINFO info;
				int ItemNum;
				DWORD dwPos;
				POINT MousePos;

				dwPos = GetMessagePos();
				MousePos.x = GET_X_LPARAM(dwPos);
				MousePos.y = GET_Y_LPARAM(dwPos);
				ScreenToClient(hTab,&MousePos);

				/* The cursor position will be tested to see if
				there is a tab beneath it. */
				info.pt.x	= LOWORD(lParam);
				info.pt.y	= HIWORD(lParam);

				ItemNum = TabCtrl_HitTest(m_hTabCtrl,&info);

				if(info.flags != TCHT_NOWHERE && m_bDoubleClickTabClose)
				{
					CloseTab(ItemNum);
				}
			}
			break;
	}

	return DefSubclassProc(hTab,msg,wParam,lParam);
}

void CContainer::OnInitTabMenu(WPARAM wParam)
{
	HMENU hTabMenu;
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,m_iTabMenuItem,&tcItem);

	hTabMenu = (HMENU)wParam;

	lCheckMenuItem(hTabMenu,IDM_TAB_LOCKTAB,m_TabInfo[(int)tcItem.lParam].bLocked);
	lCheckMenuItem(hTabMenu,IDM_TAB_LOCKTABANDADDRESS,m_TabInfo[(int)tcItem.lParam].bAddressLocked);
	lEnableMenuItem(hTabMenu,IDM_TAB_CLOSETAB,
		!(m_TabInfo[(int)tcItem.lParam].bLocked || m_TabInfo[(int)tcItem.lParam].bAddressLocked));
}

void CContainer::OnTabCtrlLButtonDown(WPARAM wParam,LPARAM lParam)
{
	TCHITTESTINFO info;
	int ItemNum;

	/* The cursor position will be tested to see if
	there is a tab beneath it. */
	info.pt.x	= LOWORD(lParam);
	info.pt.y	= HIWORD(lParam);

	ItemNum = TabCtrl_HitTest(m_hTabCtrl,&info);

	if(info.flags != TCHT_NOWHERE)
	{
		/* Save the bounds of the dragged tab. */
		TabCtrl_GetItemRect(m_hTabCtrl,ItemNum,&m_rcDraggedTab);

		/* Capture mouse movement exclusively until
		the mouse button is released. */
		SetCapture(m_hTabCtrl);

		m_bTabBeenDragged = TRUE;
	}
}

void CContainer::OnTabCtrlLButtonUp(void)
{
	if(GetCapture() == m_hTabCtrl)
		ReleaseCapture();

	m_bTabBeenDragged = FALSE;
}

void CContainer::OnTabCtrlMouseMove(WPARAM wParam,LPARAM lParam)
{
	/* Is a tab currently been dragged? */
	if(m_bTabBeenDragged)
	{
		TCHITTESTINFO HitTestInfo;
		POINT MousePos;
		int iSwap;
		int iSelected;

		/* Dragged tab. */
		iSelected = TabCtrl_GetCurFocus(m_hTabCtrl);

		MousePos.x	= LOWORD(lParam);
		MousePos.y	= HIWORD(lParam);

		HitTestInfo.pt		= MousePos;

		iSwap = TabCtrl_HitTest(m_hTabCtrl,&HitTestInfo);

		/* Check:
		- If the cursor is over an item.
		- If the cursor is not over the dragged item itself.
		- If the cursor has passed to the left of the dragged tab, or
		- If the cursor has passed to the right of the dragged tab. */
		if(HitTestInfo.flags != TCHT_NOWHERE &&
			iSwap != iSelected &&
			(MousePos.x < m_rcDraggedTab.left ||
			MousePos.x > m_rcDraggedTab.right))
		{
			RECT rcSwap;

			TabCtrl_GetItemRect(m_hTabCtrl,iSwap,&rcSwap);

			/* These values need to be adjusted, since
			tabs are adjusted whenever the dragged tab
			passes a boundary, not when the cursor is
			released. */
			if(MousePos.x > m_rcDraggedTab.right)
			{
				/* Cursor has gone past the right edge of
				the dragged tab. */
				m_rcDraggedTab.left		= m_rcDraggedTab.right;
				m_rcDraggedTab.right	= rcSwap.right;
			}
			else
			{
				/* Cursor has gone past the left edge of
				the dragged tab. */
				m_rcDraggedTab.right	= m_rcDraggedTab.left;
				m_rcDraggedTab.left		= rcSwap.left;
			}

			/* Swap the dragged tab with the tab the cursor
			finished up on. */
			TabCtrl_SwapItems(m_hTabCtrl,iSelected,iSwap);

			/* The index of the selected tab has now changed
			(but the actual tab/browser selected remains the
			same). */
			m_iTabSelectedItem = iSwap;
			TabCtrl_SetCurFocus(m_hTabCtrl,iSwap);
		}
	}
}

void CContainer::OnTabCtrlRButtonUp(WPARAM wParam,LPARAM lParam)
{
	TCHITTESTINFO tcHitTest;
	POINT ptCursor;
	int iTabHit;

	ptCursor.x	= LOWORD(lParam);
	ptCursor.y	= HIWORD(lParam);

	tcHitTest.pt = ptCursor;

	iTabHit = TabCtrl_HitTest(m_hTabCtrl,&tcHitTest);

	if(tcHitTest.flags != TCHT_NOWHERE)
	{
		UINT Command;

		ClientToScreen(m_hTabCtrl,&ptCursor);

		m_iTabMenuItem = iTabHit;

		Command = TrackPopupMenu(m_hTabRightClickMenu,
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERTICAL | TPM_RETURNCMD,
		ptCursor.x,ptCursor.y,0,m_hTabCtrl,NULL);

		ProcessTabCommand(Command,iTabHit);
	}
}

void CContainer::ProcessTabCommand(UINT uMenuID,int iTabHit)
{
	switch(uMenuID)
	{
		case IDM_TAB_DUPLICATETAB:
			OnDuplicateTab(iTabHit);
			break;

		case IDM_TAB_OPENPARENTINNEWTAB:
			{
				HRESULT			hr;
				LPITEMIDLIST	pidlCurrent = NULL;
				LPITEMIDLIST	pidlParent = NULL;

				pidlCurrent = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

				hr = GetVirtualParentPath(pidlCurrent,&pidlParent);

				if(SUCCEEDED(hr))
				{
					BrowseFolder(pidlParent,SBSP_ABSOLUTE,TRUE,TRUE);

					CoTaskMemFree(pidlParent);
				}

				CoTaskMemFree(pidlCurrent);
			}
			break;

		case IDM_TAB_REFRESH:
			RefreshTab(iTabHit);
			break;

		case IDM_TAB_REFRESHALL:
			RefreshAllTabs();
			break;

		case IDM_TAB_RENAMETAB:
			{
				RenameTabInfo_t rti;

				rti.pContainer	= (void *)this;
				rti.iTab		= iTabHit;
				DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_RENAMETAB),
					m_hContainer,RenameTabProcStub,(LPARAM)&rti);
			}
			break;

		case IDM_TAB_LOCKTAB:
			OnLockTab(iTabHit);
			break;

		case IDM_TAB_LOCKTABANDADDRESS:
			OnLockTabAndAddress(iTabHit);
			break;

		case IDM_TAB_CLOSEOTHERTABS:
			CloseOtherTabs(iTabHit);
			break;

		case IDM_TAB_CLOSETAB:
			CloseTab(iTabHit);
			break;

		default:
			/* Send the resulting command back to the main window for processing. */
			SendMessage(m_hContainer,WM_COMMAND,MAKEWPARAM(uMenuID,iTabHit),0);
			break;
	}
}

void CContainer::InitializeTabs(void)
{
	HIMAGELIST	himlSmall;
	TCHAR		szTabCloseTip[64];
	HRESULT		hr;

	/* The tab backing will hold the tab window. */
	CreateTabBacking();

	if(m_bForceSameTabWidth)
		TabCtrlStyles |= TCS_FIXEDWIDTH;

	m_hTabCtrl = CreateTabControl(m_hTabBacking,TabCtrlStyles);

	himlSmall = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,100);
	TabCtrl_SetImageList(m_hTabCtrl,himlSmall);

	/* Initialize the drag source helper, and use it to initialize the drop target helper. */
	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_INPROC_SERVER,
	IID_IDragSourceHelper,(LPVOID *)&m_pDragSourceHelper);

	if(SUCCEEDED(hr))
	{
		hr = m_pDragSourceHelper->QueryInterface(IID_IDropTargetHelper,(LPVOID *)&m_pDropTargetHelper);

		if(SUCCEEDED(hr))
		{
			/* Indicate that the tab control supports the dropping of items. */
			RegisterDragDrop(m_hTabCtrl,this);
		}
	}

	/* Subclass the tab control. */
	SetWindowSubclass(m_hTabCtrl,TabSubclassProcStub,0,(DWORD_PTR)this);

	/* Create the toolbar that will appear on the tab control.
	Only contains the close buton used to close tabs. */
	LoadString(g_hLanguageModule,IDS_TAB_CLOSE_TIP,
		szTabCloseTip,SIZEOF_ARRAY(szTabCloseTip));
	m_hTabWindowToolbar	= CreateTabToolbar(m_hTabBacking,TABTOOLBAR_CLOSE,szTabCloseTip);
}

void CContainer::InsertNewTab(LPITEMIDLIST pidlDirectory,int iNewTabIndex,int iTabId)
{
	TCITEM		tcItem;
	SHFILEINFO	shfi;
	ICONINFO	IconInfo;
	TCHAR		szTabText[MAX_PATH];
	TCHAR		szExpandedTabText[MAX_PATH];
	int			iImage;

	/* If no custom name is set, use the folders name. */
	if(!m_TabInfo[iTabId].bUseCustomName)
	{
		GetDisplayName(pidlDirectory,szTabText,SHGDN_INFOLDER);

		StringCchCopy(m_TabInfo[iTabId].szName,
			SIZEOF_ARRAY(m_TabInfo[iTabId].szName),szTabText);
	}

	SHGetFileInfo((LPCTSTR)pidlDirectory,0,&shfi,sizeof(shfi),
		SHGFI_PIDL|SHGFI_ICON|SHGFI_SMALLICON);

	GetIconInfo(shfi.hIcon,&IconInfo);
	iImage = ImageList_Add(TabCtrl_GetImageList(m_hTabCtrl),
		IconInfo.hbmColor,IconInfo.hbmMask);

	ReplaceCharacterWithString(m_TabInfo[iTabId].szName,szExpandedTabText,
		SIZEOF_ARRAY(szExpandedTabText),'&',_T("&&"));

	/* Tab control insertion information. The folders name will be used
	as the tab text. */
	tcItem.mask			= TCIF_TEXT|TCIF_IMAGE|TCIF_PARAM;
	tcItem.pszText		= szExpandedTabText;
	tcItem.iImage		= iImage;
	tcItem.lParam		= iTabId;

	SendMessage(m_hTabCtrl,TCM_INSERTITEM,(WPARAM)iNewTabIndex,(LPARAM)&tcItem);

	/* Clean up. */
	DeleteObject(IconInfo.hbmColor);
	DeleteObject(IconInfo.hbmMask);
	DestroyIcon(shfi.hIcon);
}

void CContainer::OnDuplicateTab(int iTab)
{
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	DuplicateTab((int)tcItem.lParam);
}

void CContainer::OnLockTab(int iTab)
{
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	OnLockTabInternal((int)tcItem.lParam);
}

void CContainer::OnLockTabInternal(int iTabId)
{
	m_TabInfo[iTabId].bLocked = !m_TabInfo[iTabId].bLocked;

	/* If the tab that was locked/unlocked is the
	currently selected tab, then the tab close
	button on the toolbar will need to be updated. */
	if(iTabId == m_iObjectIndex)
		HandleTabToolbarItemStates();
}

void CContainer::OnLockTabAndAddress(int iTab)
{
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	m_TabInfo[(int)tcItem.lParam].bAddressLocked = !m_TabInfo[(int)tcItem.lParam].bAddressLocked;

	/* If the tab that was locked/unlocked is the
	currently selected tab, then the tab close
	button on the toolbar will need to be updated. */
	if((int)tcItem.lParam == m_iObjectIndex)
		HandleTabToolbarItemStates();
}

void CContainer::HandleTabToolbarItemStates(void)
{
	int nTabs;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if(nTabs > 1 && !(m_TabInfo[m_iObjectIndex].bLocked || m_TabInfo[m_iObjectIndex].bAddressLocked))
	{
		/* Enable the tab close button. */
		SendMessage(m_hTabWindowToolbar,TB_SETSTATE,
		TABTOOLBAR_CLOSE,TBSTATE_ENABLED);
	}
	else
	{
		/* Disable the tab close toolbar button. */
		SendMessage(m_hTabWindowToolbar,TB_SETSTATE,
		TABTOOLBAR_CLOSE,TBSTATE_INDETERMINATE);
	}
}

BOOL CContainer::OnMouseWheel(WPARAM wParam,LPARAM lParam)
{
	HWND hUpDown;
	RECT rc;
	POINT pt;
	POINTS pts;
	DWORD dwCursorPos;
	BOOL bSuccess;
	BOOL bInRect;
	int iLow;
	int iHigh;
	int iScrollPos;
	int iPos;
	short zDelta;

	zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

	GetClientRect(m_hTabCtrl,&rc);
	dwCursorPos = GetMessagePos();

	pts = MAKEPOINTS(dwCursorPos);
	pt.x = pts.x;
	pt.y = pts.y;

	ScreenToClient(m_hTabCtrl,&pt);

	bInRect = PtInRect(&rc,pt);

	if(bInRect)
	{
		hUpDown = FindWindowEx(m_hTabCtrl,NULL,UPDOWN_CLASS,NULL);

		if(hUpDown != NULL)
		{
			iPos = (int)SendMessage(hUpDown,UDM_GETPOS32,0,(LPARAM)&bSuccess);

			if(bSuccess)
			{
				iScrollPos = iPos;
				SendMessage(hUpDown,UDM_GETRANGE32,(WPARAM)&iLow,(LPARAM)&iHigh);

				if(zDelta < 0)
				{
					if(iScrollPos < iHigh)
						iScrollPos++;
				}
				else
				{
					if(iScrollPos > iLow)
						iScrollPos--;
				}

				SendMessage(m_hTabCtrl,WM_HSCROLL,MAKEWPARAM(SB_THUMBPOSITION,iScrollPos),NULL);
			}
		}
	}
	else
	{
		/* User is scrolling within the listview. */
		if(wParam & MK_CONTROL)
		{
			int  i = 0;

			/* Switch listview views. For each wheel delta
			(notch) the wheel is scrolled through, switch
			the view once. */
			for(i = 0;i < abs(zDelta / WHEEL_DELTA);i++)
			{
				CycleViewState((zDelta > 0));
			}
		}
	}

	return bInRect;
}

void CContainer::DuplicateTab(int iTabInternal)
{
	TCHAR szTabDirectory[MAX_PATH];

	m_pShellBrowser[iTabInternal]->QueryCurrentDirectory(SIZEOF_ARRAY(szTabDirectory),
		szTabDirectory);

	BrowseFolder(szTabDirectory,SBSP_ABSOLUTE,TRUE,FALSE);
}