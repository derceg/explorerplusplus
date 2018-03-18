/******************************************************************
 *
 * Project: Explorer++
 * File: TaskbarThumbnails.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * The methods in this file manage the tab proxy windows. A tab
 * proxy is created when a taskbar thumbnail needs to be shown.
 * Note that these thumbnails are only shown on Windows 7 and
 * above.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "MainResource.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/Macros.h"


namespace
{
	LRESULT CALLBACK MainWndProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
	LRESULT CALLBACK TabProxyWndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

	typedef HRESULT (STDAPICALLTYPE *DwmSetWindowAttributeProc)(HWND hwnd,DWORD dwAttribute,LPCVOID pvAttribute,DWORD cbAttribute);
	typedef HRESULT (STDAPICALLTYPE *DwmSetIconicThumbnailProc)(HWND hwnd,HBITMAP hbmp,DWORD dwSITFlags);
	typedef HRESULT (STDAPICALLTYPE *DwmSetIconicLivePreviewBitmapProc)(HWND hwnd,HBITMAP hbmp,POINT *pptClient,DWORD dwSITFlags);
	typedef HRESULT (STDAPICALLTYPE *DwmInvalidateIconicBitmapsProc)(HWND hwnd);

	struct TabProxy_t
	{
		Explorerplusplus	*pContainer;
		int					iTabId;
	};

	/* These definitions are needed to target
	Windows 7 specific features, while remaining
	compliant with Vista. They are copied directly
	from the appropriate header file. */
	static const UINT WM_DWMSENDICONICTHUMBNAIL = 0x0323;
	static const UINT WM_DWMSENDICONICLIVEPREVIEWBITMAP = 0x0326;
}

void Explorerplusplus::InitializeTaskbarThumbnails()
{
	m_bTaskbarInitialised = FALSE;

	if(!IsWindows7OrGreater())
	{
		return;
	}

	if(!m_bShowTaskbarThumbnails)
	{
		return;
	}

	m_uTaskbarButtonCreatedMessage = RegisterWindowMessage(_T("TaskbarButtonCreated"));

	ChangeWindowMessageFilter(m_uTaskbarButtonCreatedMessage, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_DWMSENDICONICTHUMBNAIL, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_DWMSENDICONICLIVEPREVIEWBITMAP, MSGFLT_ADD);

	/* Subclass the main window until the above message (TaskbarButtonCreated) is caught. */
	SetWindowSubclass(m_hContainer,MainWndProcStub,0,reinterpret_cast<DWORD_PTR>(this));
}

namespace
{
	LRESULT CALLBACK MainWndProcStub(HWND hwnd,UINT uMsg,
		WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
	{
		UNREFERENCED_PARAMETER(uIdSubclass);

		Explorerplusplus *pexpp = reinterpret_cast<Explorerplusplus *>(dwRefData);

		return pexpp->MainWndTaskbarThumbnailProc(hwnd,uMsg,wParam,lParam);
	}
}

LRESULT CALLBACK Explorerplusplus::MainWndTaskbarThumbnailProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if(uMsg == m_uTaskbarButtonCreatedMessage)
	{
		if(m_pTaskbarList != NULL)
		{
			m_pTaskbarList->Release();
		}

		CoCreateInstance(CLSID_TaskbarList,NULL,CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_pTaskbarList));
		m_pTaskbarList->HrInit();

		m_bTaskbarInitialised = TRUE;

		/* Add each of the jump list tasks. */
		SetupJumplistTasks();

		/* Register each of the tabs. */
		for(auto itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
		{
			BOOL bActive = (itr->iTabId == m_selectedTabId);

			RegisterTab(itr->hProxy,EMPTY_STRING,bActive);
			UpdateTabText(itr->iTabId);
			SetTabIcon(itr->iTabId);
		}

		RemoveWindowSubclass(hwnd,MainWndProcStub,0);

		return 0;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void Explorerplusplus::SetupJumplistTasks()
{
	TCHAR szCurrentProcess[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(),szCurrentProcess,SIZEOF_ARRAY(szCurrentProcess));

	TCHAR szName[256];
	LoadString(m_hLanguageModule,IDS_TASKS_NEWTAB,szName,SIZEOF_ARRAY(szName));

	/* New tab task. */
	JumpListTaskInformation jlti;
	jlti.pszName		= szName;
	jlti.pszPath		= szCurrentProcess;
	jlti.pszArguments	= NExplorerplusplus::JUMPLIST_TASK_NEWTAB_ARGUMENT;
	jlti.pszIconPath	= szCurrentProcess;
	jlti.iIcon			= 1;

	std::list<JumpListTaskInformation> TaskList;
	TaskList.push_back(jlti);

	AddJumpListTasks(TaskList);
}

ATOM Explorerplusplus::RegisterTabProxyClass(const TCHAR *szClassName)
{
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(wcex);
	wcex.style			= 0;
	wcex.lpfnWndProc	= TabProxyWndProcStub;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= sizeof(TabProxy_t *);
	wcex.hInstance		= GetModuleHandle(NULL);
	wcex.hIcon			= NULL;
	wcex.hIconSm		= NULL;
	wcex.hCursor		= LoadCursor(NULL,IDC_ARROW);
	wcex.hbrBackground	= NULL;
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
void Explorerplusplus::CreateTabProxy(int iTabId,BOOL bSwitchToNewTab)
{
	HWND hTabProxy;
	TabProxyInfo_t tpi;
	TCHAR szClassName[512];
	ATOM aRet;
	BOOL bValue = TRUE;

	/* If we're not running on Windows 7 or later, return without
	doing anything. */
	if(!IsWindows7OrGreater())
	{
		return;
	}

	if(!m_bShowTaskbarThumbnails)
	{
		return;
	}

	static int iCount = 0;

	StringCchPrintf(szClassName,SIZEOF_ARRAY(szClassName),_T("Explorer++TabProxy%d"),iCount++);

	aRet = RegisterTabProxyClass(szClassName);

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
					DwmSetWindowAttribute(hTabProxy,DWMWA_FORCE_ICONIC_REPRESENTATION,
						&bValue,sizeof(BOOL));

					DwmSetWindowAttribute(hTabProxy,DWMWA_HAS_ICONIC_BITMAP,
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

void Explorerplusplus::RemoveTabProxy(int iTabId)
{
	if(m_bTaskbarInitialised)
	{
		for(auto itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
		{
			if(itr->iTabId == iTabId)
			{
				m_pTaskbarList->UnregisterTab(itr->hProxy);

				TabProxy_t *ptp = reinterpret_cast<TabProxy_t *>(GetWindowLongPtr(itr->hProxy,GWLP_USERDATA));
				DestroyWindow(itr->hProxy);
				free(ptp);

				HICON hIcon = reinterpret_cast<HICON>(GetClassLongPtr(itr->hProxy,GCLP_HICONSM));
				UnregisterClass(reinterpret_cast<LPCWSTR>(MAKEWORD(itr->atomClass,0)),GetModuleHandle(0));
				DestroyIcon(hIcon);

				m_TabProxyList.erase(itr);
				break;
			}
		}
	}
}

void Explorerplusplus::InvalidateTaskbarThumbnailBitmap(int iTabId)
{
	HMODULE hDwmapi = LoadLibrary(_T("dwmapi.dll"));

	if(hDwmapi != NULL)
	{
		DwmInvalidateIconicBitmapsProc DwmInvalidateIconicBitmaps = reinterpret_cast<DwmInvalidateIconicBitmapsProc>(
			GetProcAddress(hDwmapi,"DwmInvalidateIconicBitmaps"));

		if(DwmInvalidateIconicBitmaps != NULL)
		{
			for(auto itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
			{
				if(itr->iTabId == iTabId)
				{
					DwmInvalidateIconicBitmaps(itr->hProxy);
					break;
				}
			}
		}

		FreeLibrary(hDwmapi);
	}
}

void Explorerplusplus::RegisterTab(HWND hTabProxy, const TCHAR *szDisplayName, BOOL bTabActive)
{
	/* Register and insert the tab into the current list of
	taskbar thumbnails. */
	m_pTaskbarList->RegisterTab(hTabProxy,m_hContainer);
	m_pTaskbarList->SetTabOrder(hTabProxy,NULL);

	m_pTaskbarList->SetThumbnailTooltip(hTabProxy,szDisplayName);

	if(bTabActive)
	{
		m_pTaskbarList->SetTabActive(hTabProxy,m_hContainer,0);
	}
}

namespace
{
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

		OnSelectTabById(iTabId, FALSE);
		return 0;
		break;

	case WM_SETFOCUS:
		SetFocus(m_hListView.at(iTabId));
		break;

	case WM_SYSCOMMAND:
		switch(wParam)
		{
		case SC_CLOSE:
			break;

		default:
			SendMessage(m_hListView.at(iTabId),WM_SYSCOMMAND,wParam,lParam);
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
			Gdiplus::Color color(0,0,0);
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
			if(((double) iBitmapWidth / (double) iMaxWidth) > ((double) iBitmapHeight / (double) iMaxHeight))
			{
				iWidth = iMaxWidth;
				iHeight = (int) ceil(iMaxWidth * ((double)iBitmapHeight / (double)iBitmapWidth));
			}
			else
			{
				iHeight = iMaxHeight;
				iWidth = (int) ceil(iMaxHeight * ((double) iBitmapWidth / (double) iBitmapHeight));
			}

			/* Thumbnail bitmap. */
			Gdiplus::Bitmap bmpThumbnail(iWidth,iHeight,PixelFormat32bppARGB);

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

				FreeLibrary(hDwmapi);
			}

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

				FreeLibrary(hDwmapi);
			}

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
	Gdiplus::Color color(0,0,0);
	RECT rcMain;
	RECT rcTab;

	HWND hTab = m_hListView.at(iTabId);

	GetClientRect(m_hContainer,&rcMain);
	GetClientRect(hTab,&rcTab);

	/* Main window BitBlt. */
	hdc = GetDC(m_hContainer);
	hdcSrc = CreateCompatibleDC(hdc);

	/* Any bitmap sent back to the operating system will need to be in 32-bit
	ARGB format. */
	Gdiplus::Bitmap bi(GetRectWidth(&rcMain),GetRectHeight(&rcMain),PixelFormat32bppARGB);
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
	Gdiplus::Bitmap bmpThumbnail(GetRectWidth(&rcMain),GetRectHeight(&rcMain),PixelFormat32bppARGB);

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
	Gdiplus::Color color(0,0,0);
	MENUBARINFO mbi;
	POINT pt;
	BOOL bVisible;
	RECT rcTab;

	HWND hTab = m_hListView.at(iTabId);

	hdcTab = GetDC(hTab);
	hdcTabSrc = CreateCompatibleDC(hdcTab);

	GetClientRect(hTab,&rcTab);

	Gdiplus::Bitmap bi(GetRectWidth(&rcTab),GetRectHeight(&rcTab),PixelFormat32bppARGB);
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

void Explorerplusplus::UpdateTaskbarThumbnailsForTabSelectionChange(int selectedTabId)
{
	if (!m_bTaskbarInitialised)
	{
		return;
	}

	for (const TabProxyInfo_t &tabProxyInfo : m_TabProxyList)
	{
		if (tabProxyInfo.iTabId == selectedTabId)
		{
			int nTabs;

			nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

			/* Potentially the tab may have swapped position, so
			tell the taskbar to reposition it. */
			if (m_selectedTabIndex == (nTabs - 1))
			{
				m_pTaskbarList->SetTabOrder(tabProxyInfo.hProxy, NULL);
			}
			else
			{
				TCITEM tcNextItem;
				tcNextItem.mask = TCIF_PARAM;
				TabCtrl_GetItem(m_hTabCtrl, m_selectedTabIndex + 1, &tcNextItem);

				for (const TabProxyInfo_t &tabProxyInfoNext : m_TabProxyList)
				{
					if (tabProxyInfoNext.iTabId == tcNextItem.lParam)
					{
						m_pTaskbarList->SetTabOrder(tabProxyInfo.hProxy, tabProxyInfoNext.hProxy);
					}
				}
			}

			m_pTaskbarList->SetTabActive(tabProxyInfo.hProxy, m_hContainer, 0);
			break;
		}
	}
}

void Explorerplusplus::UpdateTaskbarThumbnailTtitle(int tabId, const std::wstring &title)
{
	if (!m_bTaskbarInitialised)
	{
		return;
	}

	for (const TabProxyInfo_t &tabProxyInfo : m_TabProxyList)
	{
		if (tabProxyInfo.iTabId == tabId)
		{
			SetWindowText(tabProxyInfo.hProxy, title.c_str());
			m_pTaskbarList->SetThumbnailTooltip(tabProxyInfo.hProxy, title.c_str());
			break;
		}
	}
}

void Explorerplusplus::SetTabProxyIcon(int iTabId, HICON hIcon)
{
	for (const TabProxyInfo_t &tabProxyInfo : m_TabProxyList)
	{
		if (tabProxyInfo.iTabId == iTabId)
		{
			HICON hIconTemp = (HICON)GetClassLongPtr(tabProxyInfo.hProxy, GCLP_HICONSM);
			DestroyIcon(hIconTemp);

			hIconTemp = CopyIcon(hIcon);

			SetClassLongPtr(tabProxyInfo.hProxy, GCLP_HICONSM, (LONG_PTR)hIconTemp);
			break;
		}
	}
}