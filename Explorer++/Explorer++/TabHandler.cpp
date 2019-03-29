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
#include <boost/algorithm/string.hpp>
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

	CTabDropHandler *pTabDropHandler = new CTabDropHandler(m_hTabCtrl,this);
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
					Tab &tab = GetTabByIndex(ItemNum);
					CloseTab(tab);
				}
			}
			break;

		case WM_NCDESTROY:
			RemoveWindowSubclass(m_hTabCtrl,TabSubclassProcStub,0);
			break;
	}

	return DefSubclassProc(hTab,msg,wParam,lParam);
}

void Explorerplusplus::UpdateTabNameInWindow(const Tab &tab)
{
	std::wstring name = tab.GetName();
	boost::replace_all(name, L"&", L"&&");

	int index = GetTabIndex(tab);
	TabCtrl_SetItemText(m_hTabCtrl, index, name.c_str());
}

int Explorerplusplus::GetSelectedTabId() const
{
	return m_selectedTabId;
}

int Explorerplusplus::GetSelectedTabIndex() const
{
	return m_selectedTabIndex;
}

void Explorerplusplus::SelectTab(const Tab &tab)
{
	int index = GetTabIndex(tab);
	SelectTabAtIndex(index);
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
			CreateNewTab(FullItemPath, TabSettings(_selected = true));
		}
	}

	/* Either no items are selected, or the focused + selected
	item was not a folder; open the default tab directory. */
	if(!bFolderSelected)
	{
		hr = CreateNewTab(m_DefaultTabDirectory, TabSettings(_selected = true));

		if (FAILED(hr))
		{
			CreateNewTab(m_DefaultTabDirectoryStatic, TabSettings(_selected = true));
		}
	}
}

HRESULT Explorerplusplus::CreateNewTab(const TCHAR *TabDirectory,
	const TabSettings &tabSettings, const FolderSettings *folderSettings,
	const InitialColumns *initialColumns, int *newTabId)
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

	hr = CreateNewTab(pidl, tabSettings, folderSettings, initialColumns, newTabId);

	CoTaskMemFree(pidl);

	return hr;
}

/* Creates a new tab. If the settings argument is NULL,
the global settings will be used. */
HRESULT Explorerplusplus::CreateNewTab(LPCITEMIDLIST pidlDirectory,
	const TabSettings &tabSettings, const FolderSettings *folderSettings,
	const InitialColumns *initialColumns, int *newTabId)
{
	if (!CheckIdl(pidlDirectory) || !IsIdlDirectory(pidlDirectory))
	{
		return E_FAIL;
	}

	int tabId = m_tabIdCounter++;
	auto item = m_Tabs.emplace(std::make_pair(tabId, tabId));

	Tab &tab = item.first->second;

	if (tabSettings.locked)
	{
		tab.SetLocked(*tabSettings.locked);
	}

	if (tabSettings.addressLocked)
	{
		tab.SetAddressLocked(*tabSettings.addressLocked);
	}

	if (tabSettings.name && !tabSettings.name->empty())
	{
		tab.SetCustomName(*tabSettings.name);
	}

	tab.listView = CreateMainListView(m_hContainer,ListViewStyles);

	if (tab.listView == NULL)
	{
		return E_FAIL;
	}

	NListView::ListView_ActivateOneClickSelect(tab.listView,m_config->globalFolderSettings.oneClickActivate,
		m_config->globalFolderSettings.oneClickActivateHoverTime);

	/* Set the listview to its initial size. */
	SetListViewInitialPosition(tab.listView);

	FolderSettings folderSettingsFinal;

	if (folderSettings)
	{
		folderSettingsFinal = *folderSettings;
	}
	else
	{
		folderSettingsFinal = GetDefaultFolderSettings(pidlDirectory);
	}

	InitialColumns initialColumnsFinal;

	if (initialColumns)
	{
		initialColumnsFinal = *initialColumns;
	}
	else
	{
		initialColumnsFinal.pControlPanelColumnList = &m_ControlPanelColumnList;
		initialColumnsFinal.pMyComputerColumnList = &m_MyComputerColumnList;
		initialColumnsFinal.pMyNetworkPlacesColumnList = &m_MyNetworkPlacesColumnList;
		initialColumnsFinal.pNetworkConnectionsColumnList = &m_NetworkConnectionsColumnList;
		initialColumnsFinal.pPrintersColumnList = &m_PrintersColumnList;
		initialColumnsFinal.pRealFolderColumnList = &m_RealFolderColumnList;
		initialColumnsFinal.pRecycleBinColumnList = &m_RecycleBinColumnList;
	}

	tab.SetShellBrowser(CShellBrowser::CreateNew(m_hContainer, tab.listView, &m_config->globalFolderSettings,
		folderSettingsFinal, initialColumnsFinal));

	/* TODO: This needs to be removed. */
	SetWindowSubclass(tab.listView,ListViewProcStub,0,reinterpret_cast<DWORD_PTR>(this));

	tab.GetShellBrowser()->SetId(tab.GetId());
	tab.GetShellBrowser()->SetResourceModule(m_hLanguageModule);

	tab.GetShellBrowser()->SetInsertSorted(m_config->insertSorted);

	int index;

	if (tabSettings.index)
	{
		index = *tabSettings.index;
	}
	else
	{
		if (m_config->openNewTabNextToCurrent)
		{
			index = m_selectedTabIndex + 1;
		}
		else
		{
			index = TabCtrl_GetItemCount(m_hTabCtrl);
		}
	}

	/* Browse folder sends a message back to the main window, which
	attempts to contact the new tab (needs to be created before browsing
	the folder). */
	InsertNewTab(pidlDirectory,index,tab.GetId());

	bool selected = false;

	if (tabSettings.selected)
	{
		selected = *tabSettings.selected;
	}

	if(selected)
	{
		if(m_iPreviousTabSelectionId != -1)
		{
			m_TabSelectionHistory.push_back(m_iPreviousTabSelectionId);
		}

		/* Select the newly created tab. */
		TabCtrl_SetCurSel(m_hTabCtrl,index);

		/* Hide the previously active tab, and show the
		newly created one. */
		ShowWindow(m_hActiveListView,SW_HIDE);
		ShowWindow(tab.listView,SW_SHOW);

		m_selectedTabId			= tab.GetId();
		m_selectedTabIndex		= index;

		m_hActiveListView		= tab.listView;
		m_pActiveShellBrowser	= tab.GetShellBrowser();

		SetFocus(tab.listView);

		m_iPreviousTabSelectionId = tab.GetId();
	}

	HRESULT hr = tab.GetShellBrowser()->BrowseFolder(pidlDirectory, SBSP_ABSOLUTE);

	if (selected)
	{
		tab.GetShellBrowser()->QueryCurrentDirectory(SIZEOF_ARRAY(m_CurrentDirectory), m_CurrentDirectory);
	}

	if(hr != S_OK)
	{
		/* Folder was not browsed. Likely that the path does not exist
		(or is locked, cannot be found, etc). */
		return E_FAIL;
	}

	if (newTabId)
	{
		*newTabId = tab.GetId();
	}

	SetTabIcon(tab);

	// There's no need to manually disconnect this. Either it will be
	// disconnected when the tab is closed and the tab object (and
	// associated signal) is destroyed or when the tab is destroyed
	// during application shutdown.
	tab.AddTabUpdatedObserver(boost::bind(&Explorerplusplus::OnTabUpdated, this, _1, _2));
	m_tabCreatedSignal(tab.GetId(), selected);

	OnNavigationCompleted(tab);

	return S_OK;
}

FolderSettings Explorerplusplus::GetDefaultFolderSettings(LPCITEMIDLIST pidlDirectory) const
{
	FolderSettings folderSettings = m_config->defaultFolderSettings;
	folderSettings.sortMode = GetDefaultSortMode(pidlDirectory);

	return folderSettings;
}

boost::signals2::connection Explorerplusplus::AddTabCreatedObserver(const TabCreatedSignal::slot_type &observer)
{
	return m_tabCreatedSignal.connect(observer);
}

boost::signals2::connection Explorerplusplus::AddTabSelectedObserver(const TabSelectedSignal::slot_type &observer)
{
	return m_tabSelectedSignal.connect(observer);
}

boost::signals2::connection Explorerplusplus::AddTabMovedObserver(const TabMovedSignal::slot_type &observer)
{
	return m_tabMovedSignal.connect(observer);
}

boost::signals2::connection Explorerplusplus::AddTabUpdatedObserver(const TabUpdatedSignal::slot_type &observer)
{
	return m_tabUpdatedSignal.connect(observer);
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

			hr = CreateNewTab(szDirectory, TabSettings(_selected = true));

			if(hr == S_OK)
				nTabsCreated++;
		}
	}
	else
	{
		if(m_config->startupMode == STARTUP_PREVIOUSTABS)
			nTabsCreated = pLoadSave->LoadPreviousTabs();
	}

	if(nTabsCreated == 0)
	{
		hr = CreateNewTab(m_DefaultTabDirectory, TabSettings(_selected = true));

		if(FAILED(hr))
			hr = CreateNewTab(m_DefaultTabDirectoryStatic, TabSettings(_selected = true));

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
	SelectTabAtIndex(m_iLastSelectedTab);

	return S_OK;
}

void Explorerplusplus::SelectTabAtIndex(int index)
{
	assert(index >= 0 && index < GetNumTabs());

	int previousIndex = TabCtrl_SetCurSel(m_hTabCtrl, index);

	if (previousIndex == -1)
	{
		return;
	}

	OnTabSelectionChanged();
}

void Explorerplusplus::OnTabSelectionChanged()
{
	int index = TabCtrl_GetCurSel(m_hTabCtrl);

	if (index == -1)
	{
		throw std::runtime_error("No selected tab");
	}

	m_selectedTabIndex = index;

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

	const Tab &tab = GetTab(m_selectedTabId);

	m_hActiveListView		= tab.listView;
	m_pActiveShellBrowser	= tab.GetShellBrowser();

	/* The selected tab has changed, so update the current
	directory. Although this is not needed internally, context
	menu extensions may need the current directory to be
	set correctly. */
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(m_CurrentDirectory),
		m_CurrentDirectory);
	SetCurrentDirectory(m_CurrentDirectory);

	m_nSelected = m_pActiveShellBrowser->GetNumSelected();

	SetActiveArrangeMenuItems();
	UpdateArrangeMenuItems();

	UpdateWindowStates();

	/* Show the new listview. */
	ShowWindow(m_hActiveListView,SW_SHOW);
	SetFocus(m_hActiveListView);

	m_tabSelectedSignal(tab);

	m_iPreviousTabSelectionId = m_selectedTabId;
}

void Explorerplusplus::RefreshAllTabs(void)
{
	for (auto &item : m_Tabs)
	{
		RefreshTab(item.second);
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
			const Tab &tab = GetTabByIndex(i);
			CloseTab(tab);
		}
	}
}

void Explorerplusplus::SelectAdjacentTab(BOOL bNextTab)
{
	int nTabs = TabCtrl_GetItemCount(m_hTabCtrl);
	int newIndex = m_selectedTabIndex;

	if(bNextTab)
	{
		/* If this is the last tab in the order,
		wrap the selection back to the start. */
		if(newIndex == (nTabs - 1))
			newIndex = 0;
		else
			newIndex++;
	}
	else
	{
		/* If this is the first tab in the order,
		wrap the selection back to the end. */
		if(newIndex == 0)
			newIndex = nTabs - 1;
		else
			newIndex--;
	}

	SelectTabAtIndex(newIndex);
}

void Explorerplusplus::OnSelectTabByIndex(int iTab)
{
	int nTabs = TabCtrl_GetItemCount(m_hTabCtrl);
	int newIndex;

	if(iTab == -1)
	{
		newIndex = nTabs - 1;
	}
	else
	{
		if(iTab < nTabs)
			newIndex = iTab;
		else
			newIndex = nTabs - 1;
	}

	SelectTabAtIndex(newIndex);
}

bool Explorerplusplus::OnCloseTab(void)
{
	const Tab &tab = GetSelectedTab();
	return CloseTab(tab);
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
	if(tab.GetLocked() || tab.GetAddressLocked())
	{
		return false;
	}

	int index = GetTabIndex(tab);
	RemoveTabFromControl(index);

	m_pDirMon->StopDirectoryMonitor(tab.GetShellBrowser()->GetDirMonitorId());

	tab.GetShellBrowser()->Release();

	DestroyWindow(tab.listView);

	// This is needed, as the erase() call below will remove the element
	// from the tabs container (which will invalidate the reference
	// passed to the function).
	int tabId = tab.GetId();

	m_Tabs.erase(tab.GetId());

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

	OnTabSelectionChanged();

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

	m_TabSelectionHistory.erase(std::remove_if(m_TabSelectionHistory.begin(),m_TabSelectionHistory.end(),
		[iRemovedInternalIndex](int iHistoryInternalIndex) -> bool
		{
			return iHistoryInternalIndex == iRemovedInternalIndex;
		}),
		m_TabSelectionHistory.end());
}

HRESULT Explorerplusplus::RefreshTab(Tab &tab)
{
	PIDLPointer pidlDirectory(tab.GetShellBrowser()->QueryCurrentDirectoryIdl());

	HRESULT hr = tab.GetShellBrowser()->BrowseFolder(pidlDirectory.get(), SBSP_ABSOLUTE|SBSP_WRITENOHISTORY);

	if (SUCCEEDED(hr))
	{
		OnNavigationCompleted(tab);
	}

	return hr;
}

void Explorerplusplus::OnInitTabMenu(HMENU hMenu)
{
	const Tab &tab = GetTabByIndex(m_iTabMenuItem);

	lCheckMenuItem(hMenu, IDM_TAB_LOCKTAB, tab.GetLocked());
	lCheckMenuItem(hMenu, IDM_TAB_LOCKTABANDADDRESS, tab.GetAddressLocked());
	lEnableMenuItem(hMenu, IDM_TAB_CLOSETAB, !(tab.GetLocked() || tab.GetAddressLocked()));
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
		m_draggedTabStartIndex = ItemNum;
		m_draggedTabEndIndex = ItemNum;
	}
}

void Explorerplusplus::OnTabCtrlLButtonUp(void)
{
	if (!m_bTabBeenDragged)
	{
		return;
	}

	ReleaseCapture();

	m_bTabBeenDragged = FALSE;

	if (m_draggedTabEndIndex != m_draggedTabStartIndex)
	{
		const Tab &tab = GetTabByIndex(m_draggedTabEndIndex);
		m_tabMovedSignal(tab, m_draggedTabStartIndex, m_draggedTabEndIndex);
	}
}

void Explorerplusplus::OnTabCtrlMouseMove(POINT *pt)
{
	if (!m_bTabBeenDragged)
	{
		return;
	}

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

		m_draggedTabEndIndex = iSwap;
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
	Tab &tab = GetTabByIndex(iTabHit);

	switch(uMenuID)
	{
		case IDM_TAB_DUPLICATETAB:
			DuplicateTab(tab);
			break;

		case IDM_TAB_OPENPARENTINNEWTAB:
			{
				LPITEMIDLIST pidlCurrent = tab.GetShellBrowser()->QueryCurrentDirectoryIdl();

				LPITEMIDLIST pidlParent = NULL;
				HRESULT hr = GetVirtualParentPath(pidlCurrent, &pidlParent);

				if (SUCCEEDED(hr))
				{
					CreateNewTab(pidlParent, TabSettings(_selected = true));
					CoTaskMemFree(pidlParent);
				}

				CoTaskMemFree(pidlCurrent);
			}
			break;

		case IDM_TAB_REFRESH:
			RefreshTab(tab);
			break;

		case IDM_TAB_REFRESHALL:
			RefreshAllTabs();
			break;

		case IDM_TAB_RENAMETAB:
			{
				CRenameTabDialog RenameTabDialog(m_hLanguageModule,IDD_RENAMETAB,m_hContainer,tab.GetId(),this,this,this);
				RenameTabDialog.ShowModalDialog();
			}
			break;

		case IDM_TAB_LOCKTAB:
			OnLockTab(tab);
			break;

		case IDM_TAB_LOCKTABANDADDRESS:
			OnLockTabAndAddress(tab);
			break;

		case IDM_TAB_CLOSEOTHERTABS:
			CloseOtherTabs(iTabHit);
			break;

		case IDM_TAB_CLOSETABSTORIGHT:
			{
				int nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

				for(int i = nTabs - 1;i > iTabHit;i--)
				{
					const Tab &currentTab = GetTabByIndex(i);
					CloseTab(currentTab);
				}
			}
			break;

		case IDM_TAB_CLOSETAB:
			CloseTab(tab);
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
	std::wstring name;

	if (GetTab(iTabId).GetUseCustomName())
	{
		name = GetTab(iTabId).GetName();
	}
	else
	{
		TCHAR folderName[MAX_PATH];
		GetDisplayName(pidlDirectory, folderName, SIZEOF_ARRAY(folderName), SHGDN_INFOLDER);

		name = folderName;
	}

	boost::replace_all(name, L"&", L"&&");

	TCHAR tabText[MAX_PATH];
	StringCchCopy(tabText, SIZEOF_ARRAY(tabText), name.c_str());

	TCITEM tcItem;
	tcItem.mask			= TCIF_TEXT|TCIF_PARAM;
	tcItem.pszText		= tabText;
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

void Explorerplusplus::OnLockTab(Tab &tab)
{
	tab.SetLocked(!tab.GetLocked());
}

void Explorerplusplus::OnLockTabAndAddress(Tab &tab)
{
	tab.SetAddressLocked(!tab.GetAddressLocked());
}

void Explorerplusplus::OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
{
	switch (propertyType)
	{
	case Tab::PropertyType::LOCKED:
	case Tab::PropertyType::ADDRESS_LOCKED:
		SetTabIcon(tab);

		/* If the tab that was locked/unlocked is the
		currently selected tab, then the tab close
		button on the toolbar will need to be updated. */
		if (IsTabSelected(tab))
		{
			UpdateTabToolbar();
		}
		break;

	case Tab::PropertyType::NAME:
		UpdateTabNameInWindow(tab);
		break;
	}

	m_tabUpdatedSignal(tab, propertyType);
}

void Explorerplusplus::UpdateTabToolbar(void)
{
	int nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	const Tab &selectedTab = GetSelectedTab();

	if(nTabs > 1 && !(selectedTab.GetLocked() || selectedTab.GetAddressLocked()))
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

void Explorerplusplus::DuplicateTab(const Tab &tab)
{
	TCHAR szTabDirectory[MAX_PATH];

	tab.GetShellBrowser()->QueryCurrentDirectory(SIZEOF_ARRAY(szTabDirectory),
		szTabDirectory);

	CreateNewTab(szTabDirectory);
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

	SortMode sortMode = SortMode::Name;

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
			const Tab &tab = GetTabByIndex(iTabHit);
			CloseTab(tab);
		}
	}
}

void Explorerplusplus::OnTabCtrlGetDispInfo(LPARAM lParam)
{
	HWND			ToolTipControl;
	LPNMTTDISPINFO	lpnmtdi;
	NMHDR			*nmhdr = NULL;
	static TCHAR	szTabToolTip[512];

	lpnmtdi = (LPNMTTDISPINFO)lParam;
	nmhdr = &lpnmtdi->hdr;

	ToolTipControl = (HWND)SendMessage(m_hTabCtrl, TCM_GETTOOLTIPS, 0, 0);

	if (nmhdr->hwndFrom == ToolTipControl)
	{
		const Tab &tab = GetTabByIndex(static_cast<int>(nmhdr->idFrom));
		tab.GetShellBrowser()->QueryCurrentDirectory(SIZEOF_ARRAY(szTabToolTip),
			szTabToolTip);

		lpnmtdi->lpszText = szTabToolTip;
	}
}

Tab &Explorerplusplus::GetTab(int tabId)
{
	return m_Tabs.at(tabId);
}

Tab *Explorerplusplus::GetTabOptional(int tabId)
{
	auto itr = m_Tabs.find(tabId);

	if (itr == m_Tabs.end())
	{
		return nullptr;
	}

	return &itr->second;
}

Tab &Explorerplusplus::GetSelectedTab()
{
	int index = TabCtrl_GetCurSel(m_hTabCtrl);

	if (index == -1)
	{
		throw std::runtime_error("No selected tab");
	}

	return GetTabByIndex(index);
}

bool Explorerplusplus::IsTabSelected(const Tab &tab)
{
	const Tab &selectedTab = GetSelectedTab();
	return tab.GetId() == selectedTab.GetId();
}

Tab &Explorerplusplus::GetTabByIndex(int index)
{
	TCITEM tcItem;
	tcItem.mask = TCIF_PARAM;
	BOOL res = TabCtrl_GetItem(m_hTabCtrl, index, &tcItem);

	if (!res)
	{
		throw std::runtime_error("Tab lookup failed");
	}

	return GetTab(static_cast<int>(tcItem.lParam));
}

int Explorerplusplus::GetTabIndex(const Tab &tab)
{
	int numTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	for (int i = 0; i < numTabs; i++)
	{
		TCITEM tcItem;
		tcItem.mask = TCIF_PARAM;
		BOOL res = TabCtrl_GetItem(m_hTabCtrl, i, &tcItem);

		if (res && (tcItem.lParam == tab.GetId()))
		{
			return i;
		}
	}

	// All internal tab objects should have an index.
	throw std::runtime_error("Couldn't determine index for tab");
}

int Explorerplusplus::GetNumTabs() const
{
	return static_cast<int>(m_Tabs.size());
}

int Explorerplusplus::MoveTab(const Tab &tab, int newIndex)
{
	int index = GetTabIndex(tab);
	return TabCtrl_MoveItem(m_hTabCtrl, index, newIndex);
}

const std::unordered_map<int, Tab> &Explorerplusplus::GetAllTabs() const
{
	return m_Tabs;
}