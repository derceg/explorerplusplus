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
#include "Explorer++.h"
#include "RenameTabDialog.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Bookmark.h"
#include "../Helper/ShellHelper.h"
#include "MainResource.h"


DWORD ListViewStyles		=	WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
								LVS_ICON|LVS_EDITLABELS|LVS_SHOWSELALWAYS|LVS_SHAREIMAGELISTS|
								LVS_AUTOARRANGE|WS_TABSTOP|LVS_ALIGNTOP;

UINT TabCtrlStyles			=	WS_VISIBLE|WS_CHILD|TCS_FOCUSNEVER|TCS_SINGLELINE|
								TCS_TOOLTIPS|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;

extern LRESULT CALLBACK	ListViewSubclassProcStub(HWND ListView,UINT msg,WPARAM wParam,LPARAM lParam);
extern LRESULT	(CALLBACK *DefaultListViewProc)(HWND,UINT,WPARAM,LPARAM);

void Explorerplusplus::InitializeTabMap(void)
{
	int i = 0;

	for(i = 0;i < MAX_TABS;i++)
	{
		m_uTabMap[i] = 0;
	}
}

void Explorerplusplus::ReleaseTabId(int iTabId)
{
	m_uTabMap[iTabId] = 0;
}

BOOL Explorerplusplus::CheckTabIdStatus(int iTabId)
{
	if(m_uTabMap[iTabId] == 0)
		return FALSE;

	return TRUE;
}

int Explorerplusplus::GenerateUniqueTabId(void)
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

HRESULT Explorerplusplus::CreateNewTab(TCHAR *TabDirectory,
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
HRESULT Explorerplusplus::CreateNewTab(LPITEMIDLIST pidlDirectory,
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

	ListView_ActivateOneClickSelect(m_hListView[iTabId],m_bOneClickActivate,m_OneClickActivateHoverTime);

	/* Set the listview to its initial size. */
	SetListViewInitialPosition(m_hListView[iTabId]);

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
		is.bShowFolderSizes		= m_bShowFolderSizes;
		is.bDisableFolderSizesNetworkRemovable = m_bDisableFolderSizesNetworkRemovable;
		is.bHideSystemFiles		= m_bHideSystemFilesGlobal;
		is.bHideLinkExtension	= m_bHideLinkExtensionGlobal;
		is.bHideRecycleBin		= m_bHideRecycleBinGlobal;
		is.bHideSysVolInfo		= m_bHideSysVolInfoGlobal;

		/* Check if there are any specific settings saved
		for the specified directory. */
		for each(auto ds in m_DirectorySettingsList)
		{
			if(CompareIdls(pidlDirectory,ds.pidlDirectory))
			{
				/* TODO: */
				//bFound = TRUE;

				is.SortMode				= ds.dsi.SortMode;
				is.ViewMode				= ds.dsi.ViewMode;
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
			is.SortMode				= DEFAULT_SORT_MODE;
			is.ViewMode				= m_ViewModeGlobal;
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

	pSettings->bForceSize	= m_bForceSize;
	pSettings->sdf			= m_SizeDisplayFormat;

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
	m_pShellBrowser[iTabId]->SetShowFriendlyDates(m_bShowFriendlyDatesGlobal);
	m_pShellBrowser[iTabId]->SetInsertSorted(m_bInsertSorted);
	m_pShellBrowser[iTabId]->SetHideRecycleBin(m_bHideRecycleBinGlobal);
	m_pShellBrowser[iTabId]->SetHideSysVolInfo(m_bHideSysVolInfoGlobal);

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

	/* If we're running on Windows 7, we'll create
	a proxy window for each tab. This proxy window
	will create the taskbar thumbnail for that tab. */
	CreateTabProxy(pidlDirectory,iTabId,bSwitchToNewTab);

	return S_OK;
}

ATOM Explorerplusplus::RegisterTabProxyClass(TCHAR *szClassName,LPITEMIDLIST pidlDirectory)
{
	WNDCLASSEX wcex;

	wcex.cbSize			= sizeof(wcex);
	wcex.style			= 0;
	wcex.lpfnWndProc	= TabProxyWndProcStub;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= sizeof(TabProxy_t *);
	wcex.hInstance		= GetModuleHandle(0);
	wcex.hIcon			= NULL;
	wcex.hIconSm		= NULL;
	wcex.hCursor		= LoadCursor(NULL,IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szClassName;

	return RegisterClassEx(&wcex);
}

/* The Display Window Manager (DWM) will only interact with top-level
windows. Therefore, we'll need to create a top-level proxy window for
every tab. This top-level window will be hidden, and will handle the
thumbnail preview for the tab.
References:
http://dotnet.dzone.com/news/windows-7-taskbar-tabbed
http://channel9.msdn.com/learn/courses/Windows7/Taskbar/Win7TaskbarNative/Exercise-Experiment-with-the-New-Windows-7-Taskbar-Features/
*/
void Explorerplusplus::CreateTabProxy(LPITEMIDLIST pidlDirectory,int iTabId,BOOL bSwitchToNewTab)
{
	HWND hTabProxy;
	TabProxyInfo_t tpi;
	TCHAR szClassName[512];
	HRESULT hr;
	ATOM aRet;
	BOOL bValue = TRUE;

	/* If we're not running on Windows 7 or later, return without
	doing anything. */
	if((m_dwMajorVersion == WINDOWS_VISTA_SEVEN_MAJORVERSION &&
		m_dwMinorVersion == 0) ||
		m_dwMajorVersion < WINDOWS_VISTA_SEVEN_MAJORVERSION)
	{
		return;
	}

	if(!m_bShowTaskbarThumbnails)
	{
		return;
	}

	static int iCount = 0;

	StringCchPrintf(szClassName,SIZEOF_ARRAY(szClassName),_T("Explorer++TabProxy%d"),iCount++);

	aRet = RegisterTabProxyClass(szClassName,pidlDirectory);

	if(aRet != 0)
	{
		TabProxy_t *ptp = NULL;

		ptp = (TabProxy_t *)malloc(sizeof(TabProxy_t));

		ptp->pContainer = this;
		ptp->iTabId = iTabId;

		hTabProxy = CreateWindow(szClassName,EMPTY_STRING,WS_OVERLAPPEDWINDOW,
			0,0,0,0,NULL,NULL,GetModuleHandle(0),(LPVOID)ptp);

		if(hTabProxy != NULL)
		{
			HMODULE hDwmapi;
			DwmSetWindowAttributeProc DwmSetWindowAttribute;

			hDwmapi = LoadLibrary(_T("dwmapi.dll"));

			if(hDwmapi != NULL)
			{
				DwmSetWindowAttribute = (DwmSetWindowAttributeProc)GetProcAddress(hDwmapi,"DwmSetWindowAttribute");

				if(DwmSetWindowAttribute != NULL)
				{
					hr = DwmSetWindowAttribute(hTabProxy,DWMWA_FORCE_ICONIC_REPRESENTATION,
						&bValue,sizeof(BOOL));

					hr = DwmSetWindowAttribute(hTabProxy,DWMWA_HAS_ICONIC_BITMAP,
						&bValue,sizeof(BOOL));

					if(m_bTaskbarInitialised)
					{
						RegisterTab(hTabProxy,EMPTY_STRING,bSwitchToNewTab);
					}

					tpi.hProxy		= hTabProxy;
					tpi.iTabId		= iTabId;
					tpi.atomClass	= aRet;

					m_TabProxyList.push_back(tpi);
				}

				FreeLibrary(hDwmapi);
			}
		}
	}
}

void Explorerplusplus::RegisterTab(HWND hTabProxy,TCHAR *szDisplayName,BOOL bTabActive)
{
	/* Register and insert the tab into the current list of
	taskbar thumbnails. */
	m_pTaskbarList3->RegisterTab(hTabProxy,m_hContainer);
	m_pTaskbarList3->SetTabOrder(hTabProxy,NULL);

	m_pTaskbarList3->SetThumbnailTooltip(hTabProxy,szDisplayName);

	if(bTabActive)
	{
		m_pTaskbarList3->SetTabActive(hTabProxy,m_hContainer,0);
	}
}

LRESULT CALLBACK TabProxyWndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	TabProxy_t *ptp = (TabProxy_t *)GetWindowLongPtr(hwnd,GWLP_USERDATA);

	switch(Msg)
	{
	case WM_CREATE:
		{
			ptp = (TabProxy_t *)((CREATESTRUCT *)lParam)->lpCreateParams;

			SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)ptp);
		}
		break;
	}

	if(ptp != NULL)
		return ptp->pContainer->TabProxyWndProc(hwnd,Msg,wParam,lParam,ptp->iTabId);
	else
		return DefWindowProc(hwnd,Msg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::TabProxyWndProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam,int iTabId)
{
	switch(Msg)
	{
	case WM_ACTIVATE:
		/* Restore the main window if necessary, and switch
		to the actual tab. */
		if(IsIconic(m_hContainer))
		{
			ShowWindow(m_hContainer,SW_RESTORE);
		}

		OnSelectTab(iTabId,FALSE);
		return 0;
		break;

	case WM_SETFOCUS:
		SetFocus(m_hListView[iTabId]);
		break;

	case WM_SYSCOMMAND:
		switch(wParam)
		{
		case SC_CLOSE:
			break;

		default:
			SendMessage(m_hListView[iTabId],WM_SYSCOMMAND,wParam,lParam);
			break;
		}
		break;

	/* Generate a thumbnail of the current tab. Basic procedure:
	1. Generate a full-scale bitmap of the main window.
	2. Overlay a bitmap of the specified tab onto the main
	window bitmap.
	3. Shrink the resulting bitmap down to the correct thumbnail size.
	
	A thumbnail will be dynamically generated, provided the main window
	is not currently minimized (as we won't be able to grap a screenshot
	of it). If the main window is minimized, we'll use a cached screenshot
	of the tab (taken before the main window was minimized). */
	case WM_DWMSENDICONICTHUMBNAIL:
		{
			HDC hdc;
			HDC hdcSrc;
			HBITMAP hbmTab = NULL;
			HBITMAP hPrevBitmap;
			Color color(0,0,0);
			HRESULT hr;
			int iBitmapWidth;
			int iBitmapHeight;
			int iWidth;
			int iHeight;
			int iMaxWidth;
			int iMaxHeight;

			iMaxWidth = HIWORD(lParam);
			iMaxHeight = LOWORD(lParam);

			/* If the main window is minimized, it won't be possible
			to generate a thumbnail for any of the tabs. In that
			case, use a static 'No Preview Available' bitmap. */
			if(IsIconic(m_hContainer))
			{
				hbmTab = (HBITMAP)LoadImage(GetModuleHandle(0),MAKEINTRESOURCE(IDB_NOPREVIEWAVAILABLE),IMAGE_BITMAP,0,0,0);

				SetBitmapDimensionEx(hbmTab,223,130,NULL);
			}
			else
			{
				hbmTab = CaptureTabScreenshot(iTabId);
			}

			SIZE sz;

			GetBitmapDimensionEx(hbmTab,&sz);

			iBitmapWidth = sz.cx;
			iBitmapHeight = sz.cy;


			/* Shrink the bitmap. */
			HDC hdcThumbnailSrc;
			HBITMAP hbmThumbnail;
			POINT pt;

			hdc = GetDC(m_hContainer);
			hdcSrc = CreateCompatibleDC(hdc);

			SelectObject(hdcSrc,hbmTab);

			hdcThumbnailSrc = CreateCompatibleDC(hdc);

			/* If the current height of the main window
			is less than the width, we'll create a thumbnail
			of maximum width; else maximum height. */
			if((iBitmapWidth / iMaxWidth) > (iBitmapHeight / iMaxHeight))
			{
				iWidth = iMaxWidth;
				iHeight = iMaxWidth * iBitmapHeight / iBitmapWidth;
			}
			else
			{
				iHeight = iMaxHeight;
				iWidth = iMaxHeight * iBitmapWidth / iBitmapHeight;
			}

			/* Thumbnail bitmap. */
			Bitmap bmpThumbnail(iWidth,iHeight,PixelFormat32bppARGB);

			bmpThumbnail.GetHBITMAP(color,&hbmThumbnail);

			hPrevBitmap = (HBITMAP)SelectObject(hdcThumbnailSrc,hbmThumbnail);

			/* Finally, shrink the full-scale bitmap down into a thumbnail. */
			SetStretchBltMode(hdcThumbnailSrc,HALFTONE);
			SetBrushOrgEx(hdcThumbnailSrc,0,0,&pt);
			StretchBlt(hdcThumbnailSrc,0,0,iWidth,iHeight,hdcSrc,0,0,iBitmapWidth,iBitmapHeight,SRCCOPY);

			SelectObject(hdcThumbnailSrc,hPrevBitmap);
			DeleteDC(hdcThumbnailSrc);

			HMODULE hDwmapi;
			DwmSetIconicThumbnailProc DwmSetIconicThumbnail;

			hDwmapi = LoadLibrary(_T("dwmapi.dll"));

			if(hDwmapi != NULL)
			{
				DwmSetIconicThumbnail = (DwmSetIconicThumbnailProc)GetProcAddress(hDwmapi,"DwmSetIconicThumbnail");

				if(DwmSetIconicThumbnail != NULL)
				{
					hr = DwmSetIconicThumbnail(hwnd,hbmThumbnail,0);
				}
			}

			FreeLibrary(hDwmapi);

			/* Delete the thumbnail bitmap. */
			DeleteObject(hbmTab);
			SelectObject(hdcSrc,hPrevBitmap);
			DeleteObject(hbmThumbnail);
			DeleteDC(hdcSrc);
			ReleaseDC(m_hContainer,hdc);

			return 0;
		}
		break;

	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
		{
			HMODULE hDwmapi;
			TabPreviewInfo_t tpi;

			DwmSetIconicLivePreviewBitmapProc DwmSetIconicLivePreviewBitmap;

			tpi.hbm = NULL;

			if(IsIconic(m_hContainer))
			{
				/* TODO: Show an image here... */
			}
			else
			{
				GetTabLivePreviewBitmap(iTabId,&tpi);
			}

			hDwmapi = LoadLibrary(_T("dwmapi.dll"));

			if(hDwmapi != NULL)
			{
				DwmSetIconicLivePreviewBitmap = (DwmSetIconicLivePreviewBitmapProc)GetProcAddress(hDwmapi,"DwmSetIconicLivePreviewBitmap");

				if(DwmSetIconicLivePreviewBitmap != NULL)
				{
					DwmSetIconicLivePreviewBitmap(hwnd,tpi.hbm,&tpi.ptOrigin,0);
				}
			}

			FreeLibrary(hDwmapi);

			if(tpi.hbm != NULL)
			{
				DeleteObject(tpi.hbm);
			}

			return 0;
		}
		break;

	case WM_CLOSE:
		{
			TCITEM tcItem;
			int nTabs;
			int i = 0;

			nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

			if(nTabs == 1)
			{
				/* If this is the last tab, we'll close
				the whole application. */
				SendMessage(m_hContainer,WM_CLOSE,0,0);
			}
			else
			{
				for(i = 0;i < nTabs;i++)
				{
					tcItem.mask = TCIF_PARAM;
					TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

					if((int)tcItem.lParam == iTabId)
					{
						/* Close the tab... */
						CloseTab(i);
						break;
					}
				}
			}
		}
		break;
	}

	return DefWindowProc(hwnd,Msg,wParam,lParam);
}

HBITMAP Explorerplusplus::CaptureTabScreenshot(int iTabId)
{
	HDC hdc;
	HDC hdcSrc;
	HBITMAP hBitmap;
	HBITMAP hPrevBitmap;
	Color color(0,0,0);
	RECT rcMain;
	RECT rcTab;

	HWND hTab = m_hListView[iTabId];

	GetClientRect(m_hContainer,&rcMain);
	GetClientRect(hTab,&rcTab);


	/* Main window BitBlt. */
	hdc = GetDC(m_hContainer);
	hdcSrc = CreateCompatibleDC(hdc);

	/* Any bitmap sent back to the operating system will need to be in 32-bit
	ARGB format. */
	Bitmap bi(GetRectWidth(&rcMain),GetRectHeight(&rcMain),PixelFormat32bppARGB);
	bi.GetHBITMAP(color,&hBitmap);

	/* BitBlt the main window into the bitmap. */
	hPrevBitmap = (HBITMAP)SelectObject(hdcSrc,hBitmap);
	BitBlt(hdcSrc,0,0,GetRectWidth(&rcMain),GetRectHeight(&rcMain),hdc,0,0,SRCCOPY);


	/* Now BitBlt the tab onto the main window. */
	HDC hdcTab;
	HDC hdcTabSrc;
	HBITMAP hbmTab;
	HBITMAP hbmTabPrev;
	BOOL bVisible;

	hdcTab = GetDC(hTab);
	hdcTabSrc = CreateCompatibleDC(hdcTab);
	hbmTab = CreateCompatibleBitmap(hdcTab,GetRectWidth(&rcTab),GetRectHeight(&rcTab));

	hbmTabPrev = (HBITMAP)SelectObject(hdcTabSrc,hbmTab);

	bVisible = IsWindowVisible(hTab);

	if(!bVisible)
	{
		ShowWindow(hTab,SW_SHOW);
	}

	PrintWindow(hTab,hdcTabSrc,PW_CLIENTONLY);

	if(!bVisible)
	{
		ShowWindow(hTab,SW_HIDE);
	}

	MapWindowPoints(hTab,m_hContainer,(LPPOINT)&rcTab,2);
	BitBlt(hdcSrc,rcTab.left,rcTab.top,GetRectWidth(&rcTab),GetRectHeight(&rcTab),hdcTabSrc,0,0,SRCCOPY);

	SelectObject(hdcTabSrc,hbmTabPrev);
	DeleteObject(hbmTab);
	DeleteDC(hdcTabSrc);
	ReleaseDC(hTab,hdcTab);


	/* Shrink the bitmap. */
	HDC hdcThumbnailSrc;
	HBITMAP hbmThumbnail;
	POINT pt;

	hdcThumbnailSrc = CreateCompatibleDC(hdc);

	/* Thumbnail bitmap. */
	Bitmap bmpThumbnail(GetRectWidth(&rcMain),GetRectHeight(&rcMain),PixelFormat32bppARGB);

	bmpThumbnail.GetHBITMAP(color,&hbmThumbnail);

	hPrevBitmap = (HBITMAP)SelectObject(hdcThumbnailSrc,hbmThumbnail);

	/* Finally, shrink the full-scale bitmap down into a thumbnail. */
	SetStretchBltMode(hdcThumbnailSrc,HALFTONE);
	SetBrushOrgEx(hdcThumbnailSrc,0,0,&pt);
	BitBlt(hdcThumbnailSrc,0,0,GetRectWidth(&rcMain),GetRectHeight(&rcMain),hdcSrc,0,0,SRCCOPY);

	SetBitmapDimensionEx(hbmThumbnail,GetRectWidth(&rcMain),GetRectHeight(&rcMain),NULL);

	SelectObject(hdcThumbnailSrc,hPrevBitmap);
	DeleteDC(hdcThumbnailSrc);

	DeleteObject(hBitmap);

	SelectObject(hdcSrc,hPrevBitmap);

	DeleteDC(hdcSrc);
	ReleaseDC(m_hContainer,hdc);

	return hbmThumbnail;
}

/* It is up to the caller to delete the bitmap returned by this method. */
void Explorerplusplus::GetTabLivePreviewBitmap(int iTabId,TabPreviewInfo_t *ptpi)
{
	HDC hdcTab;
	HDC hdcTabSrc;
	HBITMAP hbmTab;
	HBITMAP hbmTabPrev;
	Color color(0,0,0);
	MENUBARINFO mbi;
	POINT pt;
	BOOL bVisible;
	RECT rcTab;

	HWND hTab = m_hListView[iTabId];

	hdcTab = GetDC(hTab);
	hdcTabSrc = CreateCompatibleDC(hdcTab);

	GetClientRect(hTab,&rcTab);

	Bitmap bi(GetRectWidth(&rcTab),GetRectHeight(&rcTab),PixelFormat32bppARGB);
	bi.GetHBITMAP(color,&hbmTab);

	hbmTabPrev = (HBITMAP)SelectObject(hdcTabSrc,hbmTab);

	bVisible = IsWindowVisible(hTab);

	if(!bVisible)
	{
		ShowWindow(hTab,SW_SHOW);
	}

	PrintWindow(hTab,hdcTabSrc,PW_CLIENTONLY);

	if(!bVisible)
	{
		ShowWindow(hTab,SW_HIDE);
	}

	SetStretchBltMode(hdcTabSrc,HALFTONE);
	SetBrushOrgEx(hdcTabSrc,0,0,&pt);
	StretchBlt(hdcTabSrc,0,0,GetRectWidth(&rcTab),GetRectHeight(&rcTab),hdcTabSrc,
		0,0,GetRectWidth(&rcTab),GetRectHeight(&rcTab),SRCCOPY);

	MapWindowPoints(hTab,m_hContainer,(LPPOINT)&rcTab,2);

	mbi.cbSize	 = sizeof(mbi);
	GetMenuBarInfo(m_hContainer,OBJID_MENU,0,&mbi);

	/* The operating system will automatically
	draw the main window. Therefore, we'll just shift
	the tab into it's proper position. */
	ptpi->ptOrigin.x = rcTab.left;

	/* Need to include the menu bar in the offset. */
	ptpi->ptOrigin.y = rcTab.top + mbi.rcBar.bottom - mbi.rcBar.top;

	ptpi->hbm = hbmTab;
	ptpi->iTabId = iTabId;

	SelectObject(hdcTabSrc,hbmTabPrev);
	DeleteDC(hdcTabSrc);
	ReleaseDC(hTab,hdcTab);
}

void Explorerplusplus::OnTabChangeInternal(BOOL bSetFocus)
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

	/* Inform the taskbar that this tab has become active. */
	if(m_bTaskbarInitialised)
	{
		list<TabProxyInfo_t>::iterator itr;

		for(itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
		{
			if(itr->iTabId == m_iObjectIndex)
			{
				TCITEM tcItem;
				int nTabs;

				nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

				/* POtentially the tab may have swapped position, so
				tell the taskbar to reposition it. */
				if(m_iTabSelectedItem == (nTabs - 1))
				{
					m_pTaskbarList3->SetTabOrder(itr->hProxy,NULL);
				}
				else
				{
					list<TabProxyInfo_t>::iterator itrNext;

					tcItem.mask = TCIF_PARAM;
					TabCtrl_GetItem(m_hTabCtrl,m_iTabSelectedItem + 1,&tcItem);

					for(itrNext = m_TabProxyList.begin();itrNext != m_TabProxyList.end();itrNext++)
					{
						if(itrNext->iTabId == (int)tcItem.lParam)
						{
							m_pTaskbarList3->SetTabOrder(itr->hProxy,itrNext->hProxy);
						}
					}
				}

				m_pTaskbarList3->SetTabActive(itr->hProxy,m_hContainer,0);
				break;
			}
		}
	}

	if(bSetFocus)
	{
		SetFocus(m_hActiveListView);
	}
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
			CloseTab(i);
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

	OnTabChangeInternal(TRUE);
}

void Explorerplusplus::OnSelectTab(int iTab)
{
	return OnSelectTab(iTab,TRUE);
}

void Explorerplusplus::OnSelectTab(int iTab,BOOL bSetFocus)
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

	OnTabChangeInternal(bSetFocus);
}

HRESULT Explorerplusplus::OnCloseTab(void)
{
	int iCurrentTab;

	iCurrentTab = TabCtrl_GetCurSel(m_hTabCtrl);

	return CloseTab(iCurrentTab);
}

HRESULT Explorerplusplus::CloseTab(int TabIndex)
{
	TCITEM	tcItem;
	int		NumTabs;
	int		m_iLastSelectedTab;
	int		ListViewIndex;
	int		iRemoveImage;

	m_iLastSelectedTab = TabIndex;

	NumTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	if((NumTabs == 1))
	{
		if(m_bCloseMainWindowOnTabClose)
		{
			SendMessage(m_hContainer,WM_CLOSE,0,0);
		}

		return S_OK;
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

		OnTabChangeInternal(TRUE);
	}
	else
	{
		TabCtrl_DeleteItem(m_hTabCtrl,TabIndex);

		if(TabIndex < m_iTabSelectedItem)
			m_iTabSelectedItem--;
	}

	/* Remove the tabs image from the image list. */
	TabCtrl_RemoveImage(m_hTabCtrl,iRemoveImage);

	list<TabProxyInfo_t>::iterator itr;

	if(m_bTaskbarInitialised)
	{
		for(itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
		{
			if(itr->iTabId == TabIndex)
			{
				HICON hIcon;

				m_pTaskbarList3->UnregisterTab(itr->hProxy);

				TabProxy_t *ptp = (TabProxy_t *)GetWindowLongPtr(itr->hProxy,GWLP_USERDATA);

				free(ptp);

				DestroyWindow(itr->hProxy);

				hIcon = (HICON)GetClassLongPtr(itr->hProxy,GCLP_HICONSM);
				DestroyIcon(hIcon);

				UnregisterClass((LPCWSTR)MAKEWORD(itr->atomClass,0),GetModuleHandle(0));

				m_TabProxyList.erase(itr);
				break;
			}
		}
	}

	m_pDirMon->StopDirectoryMonitor(m_pShellBrowser[ListViewIndex]->GetDirMonitorId());

	m_pFolderView[ListViewIndex]->SetTerminationStatus();
	DestroyWindow(m_hListView[ListViewIndex]);

	m_pShellBrowser[ListViewIndex]->Release();
	m_pShellBrowser[ListViewIndex] = NULL;
	m_pFolderView[ListViewIndex]->Release();
	m_pFolderView[ListViewIndex] = NULL;

	HandleTabToolbarItemStates();

	if(!m_bAlwaysShowTabBar)
	{
		if(TabCtrl_GetItemCount(m_hTabCtrl) == 1)
		{
			RECT rc;

			m_bShowTabBar = FALSE;

			GetClientRect(m_hContainer,&rc);

			SendMessage(m_hContainer,WM_SIZE,SIZE_RESTORED,
				(LPARAM)MAKELPARAM(rc.right,rc.bottom));
		}
	}

	return S_OK;
}

void Explorerplusplus::RefreshTab(int iTabId)
{
	LPITEMIDLIST pidlDirectory = NULL;
	HRESULT hr;

	pidlDirectory = m_pShellBrowser[iTabId]->QueryCurrentDirectoryIdl();

	hr = m_pShellBrowser[iTabId]->BrowseFolder(pidlDirectory,
		SBSP_SAMEBROWSER|SBSP_ABSOLUTE|SBSP_WRITENOHISTORY);

	if(SUCCEEDED(hr))
		OnDirChanged(iTabId);

	CoTaskMemFree(pidlDirectory);
}

void Explorerplusplus::OnTabSelectionChange(void)
{
	m_iTabSelectedItem = TabCtrl_GetCurSel(m_hTabCtrl);

	OnTabChangeInternal(TRUE);
}

LRESULT CALLBACK TabSubclassProcStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->TabSubclassProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::TabSubclassProc(HWND hTab,UINT msg,WPARAM wParam,LPARAM lParam)
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

void Explorerplusplus::OnInitTabMenu(WPARAM wParam)
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

void Explorerplusplus::OnTabCtrlLButtonDown(WPARAM wParam,LPARAM lParam)
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

void Explorerplusplus::OnTabCtrlLButtonUp(void)
{
	if(GetCapture() == m_hTabCtrl)
		ReleaseCapture();

	m_bTabBeenDragged = FALSE;
}

void Explorerplusplus::OnTabCtrlMouseMove(WPARAM wParam,LPARAM lParam)
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

void Explorerplusplus::OnTabCtrlRButtonUp(WPARAM wParam,LPARAM lParam)
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

void Explorerplusplus::ProcessTabCommand(UINT uMenuID,int iTabHit)
{
	switch(uMenuID)
	{
		case IDM_TAB_DUPLICATETAB:
			OnDuplicateTab(iTabHit);
			break;

		case IDM_TAB_OPENPARENTINNEWTAB:
			{
				TCITEM tcItem;
				tcItem.mask = TCIF_PARAM;
				TabCtrl_GetItem(m_hTabCtrl,iTabHit,&tcItem);

				LPITEMIDLIST pidlCurrent = m_pShellBrowser[static_cast<int>(tcItem.lParam)]->QueryCurrentDirectoryIdl();

				LPITEMIDLIST pidlParent = NULL;
				HRESULT hr = GetVirtualParentPath(pidlCurrent,&pidlParent);

				if(SUCCEEDED(hr))
				{
					BrowseFolder(pidlParent,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
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
				CRenameTabDialog RenameTabDialog(g_hLanguageModule,IDD_RENAMETAB,m_hContainer,this);

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
					CloseTab(i);
				}
			}
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

void Explorerplusplus::InitializeTabs(void)
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
	AddDefaultTabIcons(himlSmall);
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

void Explorerplusplus::AddDefaultTabIcons(HIMAGELIST himlTab)
{
	HIMAGELIST himlTemp;
	HBITMAP hBitmap;
	ICONINFO IconInfo;

	himlTemp = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);

	hBitmap = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));

	ImageList_Add(himlTemp,hBitmap,NULL);
	GetIconInfo(ImageList_GetIcon(himlTemp,SHELLIMAGES_LOCK,
		ILD_TRANSPARENT),&IconInfo);
	ImageList_Add(himlTab,IconInfo.hbmColor,IconInfo.hbmMask);

	DeleteObject(IconInfo.hbmColor);
	DeleteObject(IconInfo.hbmMask);
	ImageList_Destroy(himlTemp);
}

void Explorerplusplus::InsertNewTab(LPITEMIDLIST pidlDirectory,int iNewTabIndex,int iTabId)
{
	TCITEM		tcItem;
	TCHAR		szTabText[MAX_PATH];
	TCHAR		szExpandedTabText[MAX_PATH];

	/* If no custom name is set, use the folders name. */
	if(!m_TabInfo[iTabId].bUseCustomName)
	{
		GetDisplayName(pidlDirectory,szTabText,SHGDN_INFOLDER);

		StringCchCopy(m_TabInfo[iTabId].szName,
			SIZEOF_ARRAY(m_TabInfo[iTabId].szName),szTabText);
	}

	ReplaceCharacterWithString(m_TabInfo[iTabId].szName,szExpandedTabText,
		SIZEOF_ARRAY(szExpandedTabText),'&',_T("&&"));

	/* Tab control insertion information. The folders name will be used
	as the tab text. */
	tcItem.mask			= TCIF_TEXT|TCIF_PARAM;
	tcItem.pszText		= szExpandedTabText;
	tcItem.lParam		= iTabId;

	SendMessage(m_hTabCtrl,TCM_INSERTITEM,(WPARAM)iNewTabIndex,(LPARAM)&tcItem);

	SetTabIcon(iNewTabIndex,iTabId,pidlDirectory);

	if(!m_bAlwaysShowTabBar)
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
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	DuplicateTab((int)tcItem.lParam);
}

void Explorerplusplus::OnLockTab(int iTab)
{
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	OnLockTabInternal(iTab,(int)tcItem.lParam);
}

void Explorerplusplus::OnLockTabInternal(int iTab,int iTabId)
{
	m_TabInfo[iTabId].bLocked = !m_TabInfo[iTabId].bLocked;

	/* The "Lock Tab" and "Lock Tab and Address" options
	are mutually exclusive. */
	if(m_TabInfo[iTabId].bLocked)
	{
		m_TabInfo[iTabId].bAddressLocked = FALSE;
	}

	SetTabIcon(iTab,iTabId);

	/* If the tab that was locked/unlocked is the
	currently selected tab, then the tab close
	button on the toolbar will need to be updated. */
	if(iTabId == m_iObjectIndex)
		HandleTabToolbarItemStates();
}

void Explorerplusplus::OnLockTabAndAddress(int iTab)
{
	TCITEM tcItem;

	tcItem.mask = TCIF_PARAM;
	TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	m_TabInfo[(int)tcItem.lParam].bAddressLocked = !m_TabInfo[(int)tcItem.lParam].bAddressLocked;

	if(m_TabInfo[(int)tcItem.lParam].bAddressLocked)
	{
		m_TabInfo[(int)tcItem.lParam].bLocked = FALSE;
	}

	SetTabIcon(iTab,(int)tcItem.lParam);

	/* If the tab that was locked/unlocked is the
	currently selected tab, then the tab close
	button on the toolbar will need to be updated. */
	if((int)tcItem.lParam == m_iObjectIndex)
		HandleTabToolbarItemStates();
}

void Explorerplusplus::HandleTabToolbarItemStates(void)
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

BOOL Explorerplusplus::OnMouseWheel(WPARAM wParam,LPARAM lParam)
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
		GetClientRect(m_hTreeView,&rc);

		pt.x = pts.x;
		pt.y = pts.y;

		ScreenToClient(m_hTreeView,&pt);

		bInRect = PtInRect(&rc,pt);

		if(bInRect && m_bShowFolders)
		{
			WORD wScrollType;
			int  i = 0;

			if(zDelta > 0)
				wScrollType = SB_LINEUP;
			else
				wScrollType = SB_LINEDOWN;

			for(i = 0;i < TREEVIEW_WHEEL_MULTIPLIER * abs(zDelta / WHEEL_DELTA);i++)
			{
				SendMessage(m_hTreeView,WM_VSCROLL,MAKEWORD(wScrollType,0),NULL);
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
			else if(wParam & MK_SHIFT)
			{
				if(zDelta < 0)
					OnBrowseBack();
				else
					OnBrowseForward();
			}
			else
			{
				/* TODO: http://www.explorerplusplus.com/forum/viewtopic.php?f=5&t=567 */
				WORD wScrollType;
				int i = 0;

				if(zDelta > 0)
					wScrollType = SB_LINEUP;
				else
					wScrollType = SB_LINEDOWN;

				for(i = 0;i < LISTVIEW_WHEEL_MULTIPLIER * abs(zDelta / WHEEL_DELTA);i++)
				{
					SendMessage(m_hActiveListView,WM_VSCROLL,MAKEWORD(wScrollType,0),NULL);
				}

				bInRect = TRUE;
			}
		}
	}

	return bInRect;
}

void Explorerplusplus::DuplicateTab(int iTabInternal)
{
	TCHAR szTabDirectory[MAX_PATH];

	m_pShellBrowser[iTabInternal]->QueryCurrentDirectory(SIZEOF_ARRAY(szTabDirectory),
		szTabDirectory);

	BrowseFolder(szTabDirectory,SBSP_ABSOLUTE,TRUE,FALSE,FALSE);
}

void Explorerplusplus::SetTabProxyIcon(int iTabId,HICON hIcon)
{
	list<TabProxyInfo_t>::iterator itr;

	for(itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
	{
		if(itr->iTabId == iTabId)
		{
			HICON hIconTemp;

			hIconTemp = (HICON)GetClassLongPtr(itr->hProxy,GCLP_HICONSM);
			DestroyIcon(hIconTemp);

			hIconTemp = CopyIcon(hIcon);

			SetClassLongPtr(itr->hProxy,GCLP_HICONSM,(LONG_PTR)hIconTemp);
			break;
		}
	}
}