// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "MainImages.h"
#include "MainResource.h"
#include "RenameTabDialog.h"
#include "ShellBrowser/iShellView.h"
#include "ShellBrowser/SortModes.h"
#include "TabDropHandler.h"
#include "../Helper/Controls.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/TabHelper.h"
#include <algorithm>
#include <list>


DWORD ListViewStyles		=	WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
								LVS_ICON|LVS_EDITLABELS|LVS_SHOWSELALWAYS|LVS_SHAREIMAGELISTS|
								LVS_AUTOARRANGE|WS_TABSTOP|LVS_ALIGNTOP;

UINT TabCtrlStyles			=	WS_VISIBLE|WS_CHILD|TCS_FOCUSNEVER|TCS_SINGLELINE|
								TCS_TOOLTIPS|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;

LRESULT CALLBACK TabSubclassProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

extern LRESULT CALLBACK ListViewProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

extern std::list<std::wstring> g_TabDirs;

void Explorerplusplus::InitializeTabs(void)
{
	/* The tab backing will hold the tab window. */
	CreateTabBacking();

	if(m_config->forceSameTabWidth)
	{
		TabCtrlStyles |= TCS_FIXEDWIDTH;
	}

	m_hTabCtrl = CreateTabControl(m_hTabBacking,TabCtrlStyles);

	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
	m_hTabFont = CreateFontIndirect(&ncm.lfSmCaptionFont);

	if(m_hTabFont != NULL)
	{
		SendMessage(m_hTabCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(m_hTabFont), MAKELPARAM(TRUE, 0));
	}

	m_hTabCtrlImageList = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,100);
	AddDefaultTabIcons(m_hTabCtrlImageList);
	TabCtrl_SetImageList(m_hTabCtrl, m_hTabCtrlImageList);

	m_pTabContainer = new CTabContainer(m_hTabCtrl,&m_Tabs,this);

	CTabDropHandler *pTabDropHandler = new CTabDropHandler(m_hTabCtrl,m_pTabContainer);
	RegisterDragDrop(m_hTabCtrl,pTabDropHandler);
	pTabDropHandler->Release();

	SetWindowSubclass(m_hTabCtrl,TabSubclassProcStub,0,reinterpret_cast<DWORD_PTR>(this));

	/* Create the toolbar that will appear on the tab control.
	Only contains the close button used to close tabs. */
	TCHAR szTabCloseTip[64];
	LoadString(m_hLanguageModule,IDS_TAB_CLOSE_TIP,szTabCloseTip,SIZEOF_ARRAY(szTabCloseTip));
	m_hTabWindowToolbar	= CreateTabToolbar(m_hTabBacking,TABTOOLBAR_CLOSE,szTabCloseTip);
}

LRESULT CALLBACK TabSubclassProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->TabSubclassProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::TabSubclassProc(HWND hTab,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITMENU:
			OnInitTabMenu(reinterpret_cast<HMENU>(wParam));
			break;

		case WM_MENUSELECT:
			/* Forward the message to the main window so it can
			handle menu help. */
			SendMessage(m_hContainer,WM_MENUSELECT,wParam,lParam);
			break;

		case WM_LBUTTONDOWN:
			{
				POINT pt;
				POINTSTOPOINT(pt, MAKEPOINTS(lParam));
				OnTabCtrlLButtonDown(&pt);
			}
			break;

		case WM_LBUTTONUP:
			OnTabCtrlLButtonUp();
			break;

		case WM_MOUSEMOVE:
			{
				POINT pt;
				POINTSTOPOINT(pt, MAKEPOINTS(lParam));
				OnTabCtrlMouseMove(&pt);
			}
			break;

		case WM_MBUTTONUP:
			{
				POINT pt;
				POINTSTOPOINT(pt, MAKEPOINTS(lParam));
				OnTabCtrlMButtonUp(&pt);
			}
			break;

		case WM_RBUTTONUP:
			{
				POINT pt;
				POINTSTOPOINT(pt, MAKEPOINTS(lParam));
				OnTabCtrlRButtonUp(&pt);
			}
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

				if(info.flags != TCHT_NOWHERE && m_config->doubleClickTabClose)
				{
					auto tab = GetTabByIndex(ItemNum);

					if (tab)
					{
						CloseTab(*tab);
					}
				}
			}
			break;

		case WM_NCDESTROY:
			RemoveWindowSubclass(m_hTabCtrl,TabSubclassProcStub,0);
			break;
	}

	return DefSubclassProc(hTab,msg,wParam,lParam);
}

void Explorerplusplus::SetTabName(Tab &tab,const std::wstring strName)
{
	StringCchCopy(tab.szName, SIZEOF_ARRAY(tab.szName),strName.c_str());
	tab.bUseCustomName = TRUE;

	auto index = GetTabIndex(tab);

	if (!index)
	{
		assert(false);
		return;
	}

	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = tab.szName;
	TabCtrl_SetItem(m_hTabCtrl, *index, &tcItem);
}

void Explorerplusplus::ClearTabName(Tab &tab)
{
	PIDLPointer pidlDirectory(tab.shellBrower->QueryCurrentDirectoryIdl());

	TCHAR name[MAX_PATH];
	HRESULT hr = GetDisplayName(pidlDirectory.get(), name, SIZEOF_ARRAY(name), SHGDN_INFOLDER);

	if (FAILED(hr))
	{
		return;
	}

	SetTabName(tab, name);
}

void Explorerplusplus::SetTabSelection(int Index)
{
	m_selectedTabIndex = Index;
	TabCtrl_SetCurSel(m_hTabCtrl,m_selectedTabIndex);
	OnTabChangeInternal(TRUE);
}

/*
* Creates a new tab. If a folder is selected,
* that folder is opened in a new tab, else
* the default directory is opened.
*/
void Explorerplusplus::OnNewTab()
{
	int		iSelected;
	HRESULT	hr;
	BOOL	bFolderSelected = FALSE;

	iSelected = ListView_GetNextItem(m_hActiveListView,
		-1, LVNI_FOCUSED | LVNI_SELECTED);

	if(iSelected != -1)
	{
		TCHAR FullItemPath[MAX_PATH];

		/* An item is selected, so get its full pathname. */
		m_pActiveShellBrowser->QueryFullItemName(iSelected, FullItemPath, SIZEOF_ARRAY(FullItemPath));

		/* If the selected item is a folder, open that folder
		in a new tab, else just use the default new tab directory. */
		if(PathIsDirectory(FullItemPath))
		{
			bFolderSelected = TRUE;
			CreateNewTab(FullItemPath, nullptr, nullptr, TRUE, nullptr);
		}
	}

	/* Either no items are selected, or the focused + selected
	item was not a folder; open the default tab directory. */
	if(!bFolderSelected)
	{
		hr = CreateNewTab(m_DefaultTabDirectory, nullptr, nullptr, TRUE, nullptr);

		if (FAILED(hr))
		{
			CreateNewTab(m_DefaultTabDirectoryStatic, nullptr, nullptr, TRUE, nullptr);
		}
	}
}

HRESULT Explorerplusplus::CreateNewTab(const TCHAR *TabDirectory,
InitialSettings_t *pSettings, TabSettings *pTabSettings,BOOL bSwitchToNewTab,
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

	hr = CreateNewTab(pidl,pSettings,pTabSettings,bSwitchToNewTab,pTabObjectIndex);

	CoTaskMemFree(pidl);

	return hr;
}

/* Creates a new tab. If the settings argument is NULL,
the global settings will be used. */
HRESULT Explorerplusplus::CreateNewTab(LPCITEMIDLIST pidlDirectory,
InitialSettings_t *pSettings,TabSettings *pTabSettings,BOOL bSwitchToNewTab,
int *pTabObjectIndex)
{
	UINT				uFlags;
	HRESULT				hr;
	InitialSettings_t	is;
	int					iNewTabIndex;
	int					iTabId;

	if(!CheckIdl(pidlDirectory) || !IsIdlDirectory(pidlDirectory))
		return E_FAIL;

	if(m_config->openNewTabNextToCurrent)
		iNewTabIndex = m_selectedTabIndex + 1;
	else
		iNewTabIndex = TabCtrl_GetItemCount(m_hTabCtrl);

	iTabId = m_tabIdCounter++;

	if(pTabSettings == NULL)
	{
		m_Tabs[iTabId].bLocked			= FALSE;
		m_Tabs[iTabId].bAddressLocked	= FALSE;
		m_Tabs[iTabId].bUseCustomName	= FALSE;
	}
	else
	{
		m_Tabs[iTabId].bLocked = pTabSettings->bLocked;
		m_Tabs[iTabId].bAddressLocked = pTabSettings->bAddressLocked;
		m_Tabs[iTabId].bUseCustomName = pTabSettings->bUseCustomName;

		if (pTabSettings->bUseCustomName)
		{
			StringCchCopy(m_Tabs[iTabId].szName, SIZEOF_ARRAY(m_Tabs[iTabId].szName), pTabSettings->szName);
		}
	}

	m_Tabs[iTabId].id = iTabId;

	m_Tabs[iTabId].listView	= CreateMainListView(m_hContainer,ListViewStyles);

	if(m_Tabs[iTabId].listView == NULL)
		return E_FAIL;

	NListView::ListView_ActivateOneClickSelect(m_Tabs[iTabId].listView,m_config->oneClickActivate,m_config->oneClickActivateHoverTime);

	/* Set the listview to its initial size. */
	SetListViewInitialPosition(m_Tabs[iTabId].listView);

	/* If no explicit settings are specified, use the
	global ones. */
	if(pSettings == NULL)
	{
		BOOL bFound = FALSE;

		/* These settings are program-wide. */
		is.bGridlinesActive		= m_bShowGridlinesGlobal;
		is.bShowHidden			= m_bShowHiddenGlobal;
		is.bShowInGroups		= m_bShowInGroupsGlobal;
		is.bSortAscending		= m_bSortAscendingGlobal;
		is.bAutoArrange			= m_bAutoArrangeGlobal;
		is.bShowFolderSizes		= m_config->showFolderSizes;
		is.bDisableFolderSizesNetworkRemovable = m_config->disableFolderSizesNetworkRemovable;
		is.bHideSystemFiles		= m_bHideSystemFilesGlobal;
		is.bHideLinkExtension	= m_bHideLinkExtensionGlobal;

		/* Check if there are any specific settings saved
		for the specified directory. */
		for(auto ds : m_DirectorySettingsList)
		{
			if(CompareIdls(pidlDirectory,ds.pidlDirectory))
			{
				/* TODO: */
				//bFound = TRUE;

				is.sortMode				= ds.dsi.sortMode;
				is.viewMode				= ds.dsi.viewMode;
				is.bApplyFilter			= FALSE;
				is.bFilterCaseSensitive	= FALSE;

				is.pControlPanelColumnList			= &ds.dsi.ControlPanelColumnList;
				is.pMyComputerColumnList			= &ds.dsi.MyComputerColumnList;
				is.pMyNetworkPlacesColumnList		= &ds.dsi.MyNetworkPlacesColumnList;
				is.pNetworkConnectionsColumnList	= &ds.dsi.NetworkConnectionsColumnList;
				is.pPrintersColumnList				= &ds.dsi.PrintersColumnList;
				is.pRealFolderColumnList			= &ds.dsi.RealFolderColumnList;
				is.pRecycleBinColumnList			= &ds.dsi.RecycleBinColumnList;
			}
		}

		if(bFound)
		{
			/* There are existing settings for this directory,
			so use those, rather than the defaults. */
		}
		else
		{
			is.sortMode				= GetDefaultSortMode(pidlDirectory);
			is.viewMode				= m_ViewModeGlobal;
			is.bApplyFilter			= FALSE;
			is.bFilterCaseSensitive	= FALSE;

			StringCchCopy(is.szFilter,SIZEOF_ARRAY(is.szFilter),EMPTY_STRING);

			is.pControlPanelColumnList			= &m_ControlPanelColumnList;
			is.pMyComputerColumnList			= &m_MyComputerColumnList;
			is.pMyNetworkPlacesColumnList		= &m_MyNetworkPlacesColumnList;
			is.pNetworkConnectionsColumnList	= &m_NetworkConnectionsColumnList;
			is.pPrintersColumnList				= &m_PrintersColumnList;
			is.pRealFolderColumnList			= &m_RealFolderColumnList;
			is.pRecycleBinColumnList			= &m_RecycleBinColumnList;
		}

		pSettings = &is;
	}

	pSettings->bForceSize	= m_config->forceSize;
	pSettings->sdf			= m_config->sizeDisplayFormat;

	m_Tabs[iTabId].shellBrower = CShellBrowser::CreateNew(m_hContainer, m_Tabs[iTabId].listView,pSettings);

	if(pSettings->bApplyFilter)
		NListView::ListView_SetBackgroundImage(m_Tabs[iTabId].listView,IDB_FILTERINGAPPLIED);

	/* TODO: This needs to be removed. */
	SetWindowSubclass(m_Tabs[iTabId].listView,ListViewProcStub,0,reinterpret_cast<DWORD_PTR>(this));

	m_Tabs[iTabId].shellBrower->SetId(iTabId);
	m_Tabs[iTabId].shellBrower->SetResourceModule(m_hLanguageModule);
	m_Tabs[iTabId].shellBrower->SetHideSystemFiles(m_bHideSystemFilesGlobal);
	m_Tabs[iTabId].shellBrower->SetShowExtensions(m_bShowExtensionsGlobal);
	m_Tabs[iTabId].shellBrower->SetHideLinkExtension(m_bHideLinkExtensionGlobal);
	m_Tabs[iTabId].shellBrower->SetShowFolderSizes(m_config->showFolderSizes);
	m_Tabs[iTabId].shellBrower->SetShowFriendlyDates(m_bShowFriendlyDatesGlobal);
	m_Tabs[iTabId].shellBrower->SetInsertSorted(m_config->insertSorted);

	/* Browse folder sends a message back to the main window, which
	attempts to contact the new tab (needs to be created before browsing
	the folder). */
	InsertNewTab(pidlDirectory,iNewTabIndex,iTabId);

	if(bSwitchToNewTab)
	{
		if(m_iPreviousTabSelectionId != -1)
		{
			m_TabSelectionHistory.push_back(m_iPreviousTabSelectionId);
		}

		/* Select the newly created tab. */
		TabCtrl_SetCurSel(m_hTabCtrl,iNewTabIndex);

		/* Hide the previously active tab, and show the
		newly created one. */
		ShowWindow(m_hActiveListView,SW_HIDE);
		ShowWindow(m_Tabs[iTabId].listView,SW_SHOW);

		m_selectedTabId			= iTabId;
		m_selectedTabIndex		= iNewTabIndex;

		m_hActiveListView		= m_Tabs[m_selectedTabId].listView;
		m_pActiveShellBrowser	= m_Tabs[m_selectedTabId].shellBrower;

		SetFocus(m_Tabs[iTabId].listView);

		m_iPreviousTabSelectionId = iTabId;
	}

	uFlags = SBSP_ABSOLUTE;

	/* These settings are applied to all tabs (i.e. they
	are not tab specific). Send them to the browser
	regardless of whether it loads its own settings or not. */
	PushGlobalSettingsToTab(iTabId);

	hr = m_Tabs[iTabId].shellBrower->BrowseFolder(pidlDirectory,uFlags);

	if(bSwitchToNewTab)
		m_Tabs[iTabId].shellBrower->QueryCurrentDirectory(SIZEOF_ARRAY(m_CurrentDirectory), m_CurrentDirectory);

	if(hr != S_OK)
	{
		/* Folder was not browsed. Likely that the path does not exist
		(or is locked, cannot be found, etc). */
		return E_FAIL;
	}

	if(pTabObjectIndex != NULL)
		*pTabObjectIndex = iTabId;

	SetTabIcon(m_Tabs[iTabId]);

	m_tabCreatedSignal(iTabId, bSwitchToNewTab);

	if (bSwitchToNewTab)
	{
		OnDirChanged(iTabId);
	}

	return S_OK;
}

boost::signals2::connection Explorerplusplus::AddTabCreatedObserver(const TabCreatedSignal::slot_type &observer)
{
	return m_tabCreatedSignal.connect(observer);
}

boost::signals2::connection Explorerplusplus::AddTabRemovedObserver(const TabRemovedSignal::slot_type &observer)
{
	return m_tabRemovedSignal.connect(observer);
}

HRESULT Explorerplusplus::RestoreTabs(ILoadSave *pLoadSave)
{
	TCHAR							szDirectory[MAX_PATH];
	HRESULT							hr;
	int								nTabsCreated = 0;
	int								i = 0;

	if(!g_TabDirs.empty())
	{
		for(const auto &strDirectory : g_TabDirs)
		{
			StringCchCopy(szDirectory,SIZEOF_ARRAY(szDirectory),strDirectory.c_str());

			if(lstrcmp(strDirectory.c_str(),_T("..")) == 0)
			{
				/* Get the parent of the current directory,
				and browse to it. */
				GetCurrentDirectory(SIZEOF_ARRAY(szDirectory),szDirectory);
				PathRemoveFileSpec(szDirectory);
			}
			else if(lstrcmp(strDirectory.c_str(),_T(".")) == 0)
			{
				GetCurrentDirectory(SIZEOF_ARRAY(szDirectory),szDirectory);
			}

			hr = CreateNewTab(szDirectory,NULL,NULL,TRUE,NULL);

			if(hr == S_OK)
				nTabsCreated++;
		}
	}
	else
	{
		if(m_StartupMode == STARTUP_PREVIOUSTABS)
			nTabsCreated = pLoadSave->LoadPreviousTabs();
	}

	if(nTabsCreated == 0)
	{
		hr = CreateNewTab(m_DefaultTabDirectory,NULL,NULL,TRUE,NULL);

		if(FAILED(hr))
			hr = CreateNewTab(m_DefaultTabDirectoryStatic,NULL,NULL,TRUE,NULL);

		if(hr == S_OK)
		{
			nTabsCreated++;
		}
	}

	if(nTabsCreated == 0)
	{
		/* Should never end up here. */
		return E_FAIL;
	}

	/* Tabs created on startup will NOT have
	any automatic updates. The only thing that
	needs to be done is to monitor each
	directory. The tab that is finally switched
	to will have an associated update of window
	states. */
	for(i = 0;i < nTabsCreated;i++)
	{
		TC_ITEM tcItem;

		tcItem.mask	= TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

		HandleDirectoryMonitoring((int)tcItem.lParam);
	}

	if(!m_config->alwaysShowTabBar)
	{
		if(nTabsCreated == 1)
		{
			m_bShowTabBar = FALSE;
		}
	}

	/* m_iLastSelectedTab is the tab that was selected when the
	program was last closed. */
	if(m_iLastSelectedTab >= TabCtrl_GetItemCount(m_hTabCtrl) ||
		m_iLastSelectedTab < 0)
		m_iLastSelectedTab = 0;

	/* Set the focus back to the tab that had the focus when the program
	was last closed. */
	TabCtrl_SetCurSel(m_hTabCtrl,m_iLastSelectedTab);

	m_selectedTabIndex = m_iLastSelectedTab;

	OnTabChangeInternal(TRUE);

	return S_OK;
}

void Explorerplusplus::OnTabChangeInternal(BOOL bSetFocus)
{
	if(m_iPreviousTabSelectionId != -1)
	{
		m_TabSelectionHistory.push_back(m_iPreviousTabSelectionId);
	}

	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,m_selectedTabIndex,&tcItem);

	/* Hide the old listview. */
	ShowWindow(m_hActiveListView,SW_HIDE);

	m_selectedTabId = (int)tcItem.lParam;

	m_hActiveListView		= m_Tabs.at(m_selectedTabId).listView;
	m_pActiveShellBrowser	= m_Tabs[m_selectedTabId].shellBrower;

	/* The selected tab has changed, so update the current
	directory. Although this is not needed internally, context
	menu extensions may need the current directory to be
	set correctly. */
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(m_CurrentDirectory),
		m_CurrentDirectory);
	SetCurrentDirectory(m_CurrentDirectory);

	m_nSelected = m_pActiveShellBrowser->QueryNumSelected();

	SetActiveArrangeMenuItems();
	UpdateArrangeMenuItems();

	UpdateWindowStates();

	/* Show the new listview. */
	ShowWindow(m_hActiveListView,SW_SHOW);

	/* Inform the taskbar that this tab has become active. */
	UpdateTaskbarThumbnailsForTabSelectionChange(m_selectedTabId);

	if(bSetFocus)
	{
		SetFocus(m_hActiveListView);
	}

	m_iPreviousTabSelectionId = m_selectedTabId;
}

void Explorerplusplus::RefreshAllTabs(void)
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

void Explorerplusplus::CloseOtherTabs(int iTab)
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
			auto tab = GetTabByIndex(i);

			if (tab)
			{
				CloseTab(*tab);
			}
		}
	}
}

void Explorerplusplus::SelectAdjacentTab(BOOL bNextTab)
{
	int nTabs;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if(bNextTab)
	{
		/* If this is the last tab in the order,
		wrap the selection back to the start. */
		if(m_selectedTabIndex == (nTabs - 1))
			m_selectedTabIndex = 0;
		else
			m_selectedTabIndex++;
	}
	else
	{
		/* If this is the first tab in the order,
		wrap the selection back to the end. */
		if(m_selectedTabIndex == 0)
			m_selectedTabIndex = nTabs - 1;
		else
			m_selectedTabIndex--;
	}

	TabCtrl_SetCurSel(m_hTabCtrl,m_selectedTabIndex);

	OnTabChangeInternal(TRUE);
}

void Explorerplusplus::OnSelectTab(const Tab &tab, BOOL setFocus)
{
	auto index = GetTabIndex(tab);

	if (!index)
	{
		assert(false);
		return;
	}

	OnSelectTabByIndex(*index, setFocus);
}

void Explorerplusplus::OnSelectTabByIndex(int iTab)
{
	return OnSelectTabByIndex(iTab,TRUE);
}

void Explorerplusplus::OnSelectTabByIndex(int iTab,BOOL bSetFocus)
{
	int nTabs;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if(iTab == -1)
	{
		m_selectedTabIndex = nTabs - 1;
	}
	else
	{
		if(iTab < nTabs)
			m_selectedTabIndex = iTab;
		else
			m_selectedTabIndex = nTabs - 1;
	}

	TabCtrl_SetCurSel(m_hTabCtrl,m_selectedTabIndex);

	OnTabChangeInternal(bSetFocus);
}

bool Explorerplusplus::OnCloseTab(void)
{
	auto tab = GetTabByIndex(m_selectedTabIndex);

	if (!tab)
	{
		return false;
	}

	return CloseTab(*tab);
}

bool Explorerplusplus::CloseTab(const Tab &tab)
{
	int nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if(nTabs == 1 &&
		m_config->closeMainWindowOnTabClose)
	{
		OnClose();
		return true;
	}

	/* The tab is locked. Don't close it. */
	if(tab.bLocked || tab.bAddressLocked)
	{
		return false;
	}

	auto index = GetTabIndex(tab);

	if (!index)
	{
		assert(false);
		return false;
	}

	RemoveTabFromControl(*index);
	RemoveTabProxy(tab.id);

	m_pDirMon->StopDirectoryMonitor(tab.shellBrower->GetDirMonitorId());

	tab.shellBrower->Release();

	DestroyWindow(tab.listView);

	// This is needed, as the erase() call below will remove the element
	// from the tabs container (which will invalidate the reference
	// passed to the function, unless a copy was passed).
	int tabId = tab.id;

	m_Tabs.erase(tab.id);

	if(!m_config->alwaysShowTabBar)
	{
		if(TabCtrl_GetItemCount(m_hTabCtrl) == 1)
		{
			m_bShowTabBar = FALSE;

			RECT rc;
			GetClientRect(m_hContainer,&rc);

			SendMessage(m_hContainer,WM_SIZE,SIZE_RESTORED,
				MAKELPARAM(rc.right,rc.bottom));
		}
	}

	m_tabRemovedSignal(tabId);

	return true;
}

void Explorerplusplus::RemoveTabFromControl(int iTab)
{
	int nTabs = TabCtrl_GetItemCount(m_hTabCtrl);
	int iNewTabSelection = m_selectedTabIndex;

	/* If there was a previously active tab, the focus
	should be switched back to it. */
	if(iTab == m_selectedTabIndex &&
		!m_TabSelectionHistory.empty())
	{
		for(int i = 0;i < nTabs;i++)
		{
			TCITEM tcItem;
			tcItem.mask = TCIF_PARAM;
			TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

			if(static_cast<int>(tcItem.lParam) == m_TabSelectionHistory.back())
			{
				iNewTabSelection = i;
				m_TabSelectionHistory.pop_back();
				break;
			}
		}

		/* Tabs are removed from the selection history when they
		are closed, so any previous tab must exist. */
		assert(iNewTabSelection != -1);
	}

	TCITEM tcItemRemoved;
	tcItemRemoved.mask = TCIF_IMAGE|TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItemRemoved);

	int iRemovedInternalIndex = static_cast<int>(tcItemRemoved.lParam);

	TabCtrl_DeleteItem(m_hTabCtrl,iTab);
	TabCtrl_RemoveImage(m_hTabCtrl,tcItemRemoved.iImage);

	if(iNewTabSelection > iTab ||
		iNewTabSelection == (nTabs - 1))
	{
		iNewTabSelection--;
	}

	m_selectedTabIndex = iNewTabSelection;

	TabCtrl_SetCurSel(m_hTabCtrl,m_selectedTabIndex);
	OnTabChangeInternal(TRUE);

	m_TabSelectionHistory.erase(std::remove_if(m_TabSelectionHistory.begin(),m_TabSelectionHistory.end(),
		[iRemovedInternalIndex](int iHistoryInternalIndex) -> bool
		{
			return iHistoryInternalIndex == iRemovedInternalIndex;
		}),
		m_TabSelectionHistory.end());
}

void Explorerplusplus::RefreshTab(int iTabId)
{
	LPITEMIDLIST pidlDirectory = NULL;
	HRESULT hr;

	pidlDirectory = m_Tabs[iTabId].shellBrower->QueryCurrentDirectoryIdl();

	hr = m_Tabs[iTabId].shellBrower->BrowseFolder(pidlDirectory,
		SBSP_ABSOLUTE|SBSP_WRITENOHISTORY);

	if(SUCCEEDED(hr))
		OnDirChanged(iTabId);

	CoTaskMemFree(pidlDirectory);
}

void Explorerplusplus::OnTabSelectionChange(void)
{
	m_selectedTabIndex = TabCtrl_GetCurSel(m_hTabCtrl);

	OnTabChangeInternal(TRUE);
}

void Explorerplusplus::OnInitTabMenu(HMENU hMenu)
{
	TCITEM tcItem;
	tcItem.mask = TCIF_PARAM;
	BOOL res = TabCtrl_GetItem(m_hTabCtrl,m_iTabMenuItem,&tcItem);
	assert(res);

	if(!res)
	{
		return;
	}

	int internalIndex = static_cast<int>(tcItem.lParam);

	lCheckMenuItem(hMenu, IDM_TAB_LOCKTAB, m_Tabs.at(internalIndex).bLocked);
	lCheckMenuItem(hMenu, IDM_TAB_LOCKTABANDADDRESS, m_Tabs.at(internalIndex).bAddressLocked);
	lEnableMenuItem(hMenu, IDM_TAB_CLOSETAB,
		!(m_Tabs.at(internalIndex).bLocked || m_Tabs.at(internalIndex).bAddressLocked));
}

void Explorerplusplus::OnTabCtrlLButtonDown(POINT *pt)
{
	TCHITTESTINFO info;
	info.pt = *pt;
	int ItemNum = TabCtrl_HitTest(m_hTabCtrl,&info);

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

void Explorerplusplus::OnTabCtrlLButtonUp(void)
{
	if(GetCapture() == m_hTabCtrl)
		ReleaseCapture();

	m_bTabBeenDragged = FALSE;
}

void Explorerplusplus::OnTabCtrlMouseMove(POINT *pt)
{
	/* Is a tab currently been dragged? */
	if(m_bTabBeenDragged)
	{
		/* Dragged tab. */
		int iSelected = TabCtrl_GetCurFocus(m_hTabCtrl);

		TCHITTESTINFO HitTestInfo;
		HitTestInfo.pt = *pt;
		int iSwap = TabCtrl_HitTest(m_hTabCtrl,&HitTestInfo);

		/* Check:
		- If the cursor is over an item.
		- If the cursor is not over the dragged item itself.
		- If the cursor has passed to the left of the dragged tab, or
		- If the cursor has passed to the right of the dragged tab. */
		if(HitTestInfo.flags != TCHT_NOWHERE &&
			iSwap != iSelected &&
			(pt->x < m_rcDraggedTab.left ||
			pt->x > m_rcDraggedTab.right))
		{
			RECT rcSwap;

			TabCtrl_GetItemRect(m_hTabCtrl,iSwap,&rcSwap);

			/* These values need to be adjusted, since
			tabs are adjusted whenever the dragged tab
			passes a boundary, not when the cursor is
			released. */
			if(pt->x > m_rcDraggedTab.right)
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
			m_selectedTabIndex = iSwap;
			TabCtrl_SetCurFocus(m_hTabCtrl,iSwap);
		}
	}
}

void Explorerplusplus::OnTabCtrlRButtonUp(POINT *pt)
{
	TCHITTESTINFO tcHitTest;
	tcHitTest.pt = *pt;
	int iTabHit = TabCtrl_HitTest(m_hTabCtrl,&tcHitTest);

	if(tcHitTest.flags != TCHT_NOWHERE)
	{
		POINT ptCopy = *pt;
		ClientToScreen(m_hTabCtrl,&ptCopy);

		m_iTabMenuItem = iTabHit;

		UINT Command = TrackPopupMenu(m_hTabRightClickMenu,
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERTICAL | TPM_RETURNCMD,
		ptCopy.x,ptCopy.y,0,m_hTabCtrl,NULL);

		ProcessTabCommand(Command,iTabHit);
	}
}

void Explorerplusplus::ProcessTabCommand(UINT uMenuID,int iTabHit)
{
	auto tab = GetTabByIndex(iTabHit);

	if (!tab)
	{
		return;
	}

	switch(uMenuID)
	{
		case IDM_TAB_DUPLICATETAB:
			OnDuplicateTab(iTabHit);
			break;

		case IDM_TAB_OPENPARENTINNEWTAB:
			{
				TCITEM tcItem;
				tcItem.mask = TCIF_PARAM;
				BOOL res = TabCtrl_GetItem(m_hTabCtrl,iTabHit,&tcItem);
				assert(res);

				if(res)
				{
					LPITEMIDLIST pidlCurrent = m_Tabs[static_cast<int>(tcItem.lParam)].shellBrower->QueryCurrentDirectoryIdl();

					LPITEMIDLIST pidlParent = NULL;
					HRESULT hr = GetVirtualParentPath(pidlCurrent, &pidlParent);

					if(SUCCEEDED(hr))
					{
						CreateNewTab(pidlParent, nullptr, nullptr, TRUE, nullptr);
						CoTaskMemFree(pidlParent);
					}

					CoTaskMemFree(pidlCurrent);
				}
			}
			break;

		case IDM_TAB_REFRESH:
			RefreshTab(tab->id);
			break;

		case IDM_TAB_REFRESHALL:
			RefreshAllTabs();
			break;

		case IDM_TAB_RENAMETAB:
			{
				CRenameTabDialog RenameTabDialog(m_hLanguageModule,IDD_RENAMETAB,m_hContainer,tab->id,this,this,this);
				RenameTabDialog.ShowModalDialog();
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

		case IDM_TAB_CLOSETABSTORIGHT:
			{
				int nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

				for(int i = nTabs - 1;i > iTabHit;i--)
				{
					auto currentTab = GetTabByIndex(i);

					if (currentTab)
					{
						CloseTab(*currentTab);
					}
				}
			}
			break;

		case IDM_TAB_CLOSETAB:
			CloseTab(*tab);
			break;

		default:
			/* Send the resulting command back to the main window for processing. */
			SendMessage(m_hContainer,WM_COMMAND,MAKEWPARAM(uMenuID,iTabHit),0);
			break;
	}
}

void Explorerplusplus::AddDefaultTabIcons(HIMAGELIST himlTab)
{
	HIMAGELIST himlTemp = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 48);

	HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(himlTemp,hBitmap,NULL);
	DeleteObject(hBitmap);

	ICONINFO IconInfo;
	GetIconInfo(ImageList_GetIcon(himlTemp,SHELLIMAGES_LOCK,
		ILD_TRANSPARENT),&IconInfo);
	ImageList_Add(himlTab,IconInfo.hbmColor,IconInfo.hbmMask);
	DeleteObject(IconInfo.hbmColor);
	DeleteObject(IconInfo.hbmMask);

	ImageList_Destroy(himlTemp);
}

void Explorerplusplus::InsertNewTab(LPCITEMIDLIST pidlDirectory,int iNewTabIndex,int iTabId)
{
	TCITEM		tcItem;
	TCHAR		szTabText[MAX_PATH];
	TCHAR		szExpandedTabText[MAX_PATH];

	/* If no custom name is set, use the folders name. */
	if(!m_Tabs.at(iTabId).bUseCustomName)
	{
		GetDisplayName(pidlDirectory,szTabText,SIZEOF_ARRAY(szTabText),SHGDN_INFOLDER);

		StringCchCopy(m_Tabs.at(iTabId).szName,
			SIZEOF_ARRAY(m_Tabs.at(iTabId).szName),szTabText);
	}

	ReplaceCharacterWithString(m_Tabs.at(iTabId).szName,szExpandedTabText,
		SIZEOF_ARRAY(szExpandedTabText),'&',_T("&&"));

	/* Tab control insertion information. The folders name will be used
	as the tab text. */
	tcItem.mask			= TCIF_TEXT|TCIF_PARAM;
	tcItem.pszText		= szExpandedTabText;
	tcItem.lParam		= iTabId;

	SendMessage(m_hTabCtrl,TCM_INSERTITEM,(WPARAM)iNewTabIndex,(LPARAM)&tcItem);

	if(!m_config->alwaysShowTabBar)
	{
		if(TabCtrl_GetItemCount(m_hTabCtrl) > 1)
		{
			RECT rc;

			m_bShowTabBar = TRUE;

			GetClientRect(m_hContainer,&rc);

			SendMessage(m_hContainer,WM_SIZE,SIZE_RESTORED,
				(LPARAM)MAKELPARAM(rc.right,rc.bottom));
		}
	}
}

void Explorerplusplus::OnDuplicateTab(int iTab)
{
	TCITEM tcItem;
	tcItem.mask = TCIF_PARAM;
	BOOL res = TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);
	assert(res);

	if(!res)
	{
		return;
	}

	DuplicateTab((int)tcItem.lParam);
}

void Explorerplusplus::OnLockTab(int iTab)
{
	auto tab = GetTabByIndex(iTab);

	if (!tab)
	{
		return;
	}

	LockTab(*tab, !tab->bLocked);
}

void Explorerplusplus::LockTab(Tab &tab, bool lock)
{
	tab.bLocked = lock;

	/* The "Lock Tab" and "Lock Tab and Address" options
	are mutually exclusive. */
	if(lock)
	{
		tab.bAddressLocked = FALSE;
	}

	SetTabIcon(tab);

	/* If the tab that was locked/unlocked is the
	currently selected tab, then the tab close
	button on the toolbar will need to be updated. */
	if (tab.id == m_selectedTabId)
	{
		UpdateTabToolbar();
	}
}

void Explorerplusplus::OnLockTabAndAddress(int iTab)
{
	auto tab = GetTabByIndex(iTab);

	if (!tab)
	{
		return;
	}

	LockTabAndAddress(*tab, !tab->bAddressLocked);
}

void Explorerplusplus::LockTabAndAddress(Tab &tab, bool lock)
{
	tab.bAddressLocked = lock;

	if (tab.bAddressLocked)
	{
		tab.bLocked = FALSE;
	}

	SetTabIcon(tab);

	if (tab.id == m_selectedTabId)
	{
		UpdateTabToolbar();
	}
}

void Explorerplusplus::UpdateTabToolbar(void)
{
	int nTabs;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if(nTabs > 1 && !(m_Tabs.at(m_selectedTabId).bLocked || m_Tabs.at(m_selectedTabId).bAddressLocked))
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

BOOL Explorerplusplus::OnMouseWheel(MousewheelSource_t MousewheelSource,WPARAM wParam,LPARAM lParam)
{
	short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	m_zDeltaTotal += zDelta;

	DWORD dwCursorPos = GetMessagePos();
	POINTS pts = MAKEPOINTS(dwCursorPos);

	POINT pt;
	pt.x = pts.x;
	pt.y = pts.y;

	HWND hwnd = WindowFromPoint(pt);

	BOOL bMessageHandled = FALSE;

	/* Normally, mouse wheel messages will be sent
	to the window with focus. We want to be able to
	scroll windows even if they do not have focus,
	so we'll capture the mouse wheel message and
	and forward it to the window currently underneath
	the mouse. */
	if(hwnd == m_hActiveListView)
	{
		if(wParam & MK_CONTROL)
		{
			/* Switch listview views. For each wheel delta
			(notch) the wheel is scrolled through, switch
			the view once. */
			for(int i = 0;i < abs(m_zDeltaTotal / WHEEL_DELTA);i++)
			{
				CycleViewState((m_zDeltaTotal > 0));
			}
		}
		else if(wParam & MK_SHIFT)
		{
			if(m_zDeltaTotal < 0)
			{
				for(int i = 0;i < abs(m_zDeltaTotal / WHEEL_DELTA);i++)
				{
					OnBrowseBack();
				}
			}
			else
			{
				for(int i = 0;i < abs(m_zDeltaTotal / WHEEL_DELTA);i++)
				{
					OnBrowseForward();
				}
			}
		}
		else
		{
			if(MousewheelSource != MOUSEWHEEL_SOURCE_LISTVIEW)
			{
				bMessageHandled = TRUE;
				SendMessage(m_hActiveListView,WM_MOUSEWHEEL,wParam,lParam);
			}
		}
	}
	else if(hwnd == m_hTreeView)
	{
		if(MousewheelSource != MOUSEWHEEL_SOURCE_TREEVIEW)
		{
			bMessageHandled = TRUE;
			SendMessage(m_hTreeView,WM_MOUSEWHEEL,wParam,lParam);
		}
	}
	else if(hwnd == m_hTabCtrl)
	{
		bMessageHandled = TRUE;

		HWND hUpDown = FindWindowEx(m_hTabCtrl,NULL,UPDOWN_CLASS,NULL);

		if(hUpDown != NULL)
		{
			BOOL bSuccess;
			int iPos = static_cast<int>(SendMessage(hUpDown,UDM_GETPOS32,0,reinterpret_cast<LPARAM>(&bSuccess)));

			if(bSuccess)
			{
				int iScrollPos = iPos;

				int iLow;
				int iHigh;
				SendMessage(hUpDown,UDM_GETRANGE32,reinterpret_cast<WPARAM>(&iLow),reinterpret_cast<LPARAM>(&iHigh));

				if(m_zDeltaTotal < 0)
				{
					if(iScrollPos < iHigh)
					{
						iScrollPos++;
					}
				}
				else
				{
					if(iScrollPos > iLow)
					{
						iScrollPos--;
					}
				}

				SendMessage(m_hTabCtrl,WM_HSCROLL,MAKEWPARAM(SB_THUMBPOSITION,iScrollPos),NULL);
			}
		}
	}

	if(abs(m_zDeltaTotal) >= WHEEL_DELTA)
	{
		m_zDeltaTotal = m_zDeltaTotal % WHEEL_DELTA;
	}

	return bMessageHandled;
}

void Explorerplusplus::DuplicateTab(int iTabInternal)
{
	TCHAR szTabDirectory[MAX_PATH];

	m_Tabs[iTabInternal].shellBrower->QueryCurrentDirectory(SIZEOF_ARRAY(szTabDirectory),
		szTabDirectory);

	CreateNewTab(szTabDirectory, nullptr, nullptr, FALSE, nullptr);
}

int Explorerplusplus::GetCurrentTabId() const
{
	return m_selectedTabId;
}

SortMode Explorerplusplus::GetDefaultSortMode(LPCITEMIDLIST pidlDirectory) const
{
	const std::list<Column_t> *pColumns = NULL;

	TCHAR szDirectory[MAX_PATH];
	GetDisplayName(pidlDirectory,szDirectory,SIZEOF_ARRAY(szDirectory),SHGDN_FORPARSING);

	if(CompareVirtualFolders(szDirectory,CSIDL_CONTROLS))
	{
		pColumns = &m_ControlPanelColumnList;
	}
	else if(CompareVirtualFolders(szDirectory,CSIDL_DRIVES))
	{
		pColumns = &m_MyComputerColumnList;
	}
	else if(CompareVirtualFolders(szDirectory,CSIDL_BITBUCKET))
	{
		pColumns = &m_RecycleBinColumnList;
	}
	else if(CompareVirtualFolders(szDirectory,CSIDL_PRINTERS))
	{
		pColumns = &m_PrintersColumnList;
	}
	else if(CompareVirtualFolders(szDirectory,CSIDL_CONNECTIONS))
	{
		pColumns = &m_NetworkConnectionsColumnList;
	}
	else if(CompareVirtualFolders(szDirectory,CSIDL_NETWORK))
	{
		pColumns = &m_MyNetworkPlacesColumnList;
	}
	else
	{
		pColumns = &m_RealFolderColumnList;
	}

	SortMode sortMode = FSM_NAME;

	for(const auto &Column : *pColumns)
	{
		if(Column.bChecked)
		{
			sortMode = CShellBrowser::DetermineColumnSortMode(Column.id);
			break;
		}
	}

	return sortMode;
}

void Explorerplusplus::OnTabCtrlMButtonUp(POINT *pt)
{
	/* Only close a tab if the tab control
	actually has focused (i.e. if the middle mouse
	button was clicked on the control, then the
	tab control will have focus; if it was clicked
	somewhere else, it won't). */
	if(GetFocus() == m_hTabCtrl)
	{
		TCHITTESTINFO htInfo;
		htInfo.pt = *pt;

		/* Find the tab that the click occurred over. */
		int iTabHit = TabCtrl_HitTest(m_hTabCtrl,&htInfo);

		if(iTabHit != -1)
		{
			auto tab = GetTabByIndex(iTabHit);

			if (tab)
			{
				CloseTab(*tab);
			}
		}
	}
}

void Explorerplusplus::OnTabCtrlGetDispInfo(LPARAM lParam)
{
	HWND			ToolTipControl;
	LPNMTTDISPINFO	lpnmtdi;
	NMHDR			*nmhdr = NULL;
	static TCHAR	szTabToolTip[512];
	TCITEM			tcItem;

	lpnmtdi = (LPNMTTDISPINFO)lParam;
	nmhdr = &lpnmtdi->hdr;

	ToolTipControl = (HWND)SendMessage(m_hTabCtrl, TCM_GETTOOLTIPS, 0, 0);

	if (nmhdr->hwndFrom == ToolTipControl)
	{
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl, nmhdr->idFrom, &tcItem);

		m_Tabs[(int)tcItem.lParam].shellBrower->QueryCurrentDirectory(SIZEOF_ARRAY(szTabToolTip),
			szTabToolTip);
		lpnmtdi->lpszText = szTabToolTip;
	}
}

void Explorerplusplus::PushGlobalSettingsToTab(int iTabId)
{
	GlobalSettings_t gs;	

	/* These settings are global to the whole program. */
	gs.bShowExtensions		= m_bShowExtensionsGlobal;
	gs.bShowFriendlyDates	= m_bShowFriendlyDatesGlobal;
	gs.bShowFolderSizes		= m_config->showFolderSizes;

	m_Tabs[iTabId].shellBrower->SetGlobalSettings(&gs);
}

Tab *Explorerplusplus::GetTab(int tabId)
{
	auto itr = m_Tabs.find(tabId);

	if (itr == m_Tabs.end())
	{
		return nullptr;
	}

	return &itr->second;
}

Tab *Explorerplusplus::GetTabByIndex(int index)
{
	TCITEM tcItem;
	tcItem.mask = TCIF_PARAM;
	BOOL res = TabCtrl_GetItem(m_hTabCtrl, index, &tcItem);

	if (!res)
	{
		return nullptr;
	}

	return GetTab(static_cast<int>(tcItem.lParam));
}

boost::optional<int> Explorerplusplus::GetTabIndex(const Tab &tab)
{
	int numTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	for (int i = 0; i < numTabs; i++)
	{
		TCITEM tcItem;
		tcItem.mask = TCIF_PARAM;
		BOOL res = TabCtrl_GetItem(m_hTabCtrl, i, &tcItem);

		if (res && (tcItem.lParam == tab.id))
		{
			return i;
		}
	}

	return boost::none;
}

int Explorerplusplus::GetNumTabs() const
{
	return static_cast<int>(m_Tabs.size());
}

int Explorerplusplus::MoveTab(const Tab &tab, int newIndex)
{
	auto index = GetTabIndex(tab);

	if (!index)
	{
		return -1;
	}

	return TabCtrl_MoveItem(m_hTabCtrl, *index, newIndex);
}

const std::unordered_map<int, Tab> &Explorerplusplus::GetAllTabs() const
{
	return m_Tabs;
}