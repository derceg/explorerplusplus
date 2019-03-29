// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * The methods in this file manage the tab proxy windows. A tab
 * proxy is created when a taskbar thumbnail needs to be shown.
 * Note that these thumbnails are only shown on Windows 7 and
 * above.
 */

#include "stdafx.h"
#include "TaskbarThumbnails.h"
#include "MainResource.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"

namespace
{
	typedef HRESULT (STDAPICALLTYPE *DwmSetWindowAttributeProc)(HWND hwnd,DWORD dwAttribute,LPCVOID pvAttribute,DWORD cbAttribute);
	typedef HRESULT (STDAPICALLTYPE *DwmSetIconicThumbnailProc)(HWND hwnd,HBITMAP hbmp,DWORD dwSITFlags);
	typedef HRESULT (STDAPICALLTYPE *DwmSetIconicLivePreviewBitmapProc)(HWND hwnd,HBITMAP hbmp,POINT *pptClient,DWORD dwSITFlags);
	typedef HRESULT (STDAPICALLTYPE *DwmInvalidateIconicBitmapsProc)(HWND hwnd);

	struct TabProxy_t
	{
		TaskbarThumbnails *taskbarThumbnails;
		int iTabId;
	};

	/* These definitions are needed to target
	Windows 7 specific features, while remaining
	compliant with Vista. They are copied directly
	from the appropriate header file. */
	static const UINT WM_DWMSENDICONICTHUMBNAIL = 0x0323;
	static const UINT WM_DWMSENDICONICLIVEPREVIEWBITMAP = 0x0326;
}

TaskbarThumbnails *TaskbarThumbnails::Create(IExplorerplusplus *expp, TabContainerInterface *tabContainer,
	HINSTANCE instance, std::shared_ptr<Config> config)
{
	return new TaskbarThumbnails(expp, tabContainer, instance, config);
}

TaskbarThumbnails::TaskbarThumbnails(IExplorerplusplus *expp, TabContainerInterface *tabContainer,
	HINSTANCE instance, std::shared_ptr<Config> config) :
	m_expp(expp),
	m_tabContainer(tabContainer),
	m_instance(instance),
	m_bTaskbarInitialised(false),
	m_enabled(config->showTaskbarThumbnails),
	m_pTaskbarList(nullptr)
{
	Initialize();
}

TaskbarThumbnails::~TaskbarThumbnails()
{

}

void TaskbarThumbnails::Initialize()
{
	if(!IsWindows7OrGreater())
	{
		return;
	}

	if(!m_enabled)
	{
		return;
	}

	m_uTaskbarButtonCreatedMessage = RegisterWindowMessage(_T("TaskbarButtonCreated"));

	ChangeWindowMessageFilter(m_uTaskbarButtonCreatedMessage, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_DWMSENDICONICTHUMBNAIL, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_DWMSENDICONICLIVEPREVIEWBITMAP, MSGFLT_ADD);

	/* Subclass the main window until the above message (TaskbarButtonCreated) is caught. */
	SetWindowSubclass(m_expp->GetMainWindow(),MainWndProcStub,0,reinterpret_cast<DWORD_PTR>(this));

	m_tabContainer->AddTabCreatedObserver(boost::bind(&TaskbarThumbnails::CreateTabProxy, this, _1, _2));
	m_tabContainer->AddTabSelectedObserver(boost::bind(&TaskbarThumbnails::OnTabSelectionChanged, this, _1));
	m_tabContainer->AddTabRemovedObserver(boost::bind(&TaskbarThumbnails::RemoveTabProxy, this, _1));

	m_tabContainer->AddNavigationCompletedObserver(boost::bind(&TaskbarThumbnails::OnNavigationCompleted, this, _1));
}

LRESULT CALLBACK TaskbarThumbnails::MainWndProcStub(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	TaskbarThumbnails *taskbarThumbnails = reinterpret_cast<TaskbarThumbnails *>(dwRefData);

	return taskbarThumbnails->MainWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK TaskbarThumbnails::MainWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
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
			const Tab &tab = m_tabContainer->GetTab(itr->iTabId);

			BOOL bActive = m_tabContainer->IsTabSelected(tab);

			RegisterTab(itr->hProxy,EMPTY_STRING,bActive);

			UpdateTaskbarThumbnailTtitle(tab);
			SetTabProxyIcon(tab);
		}

		RemoveWindowSubclass(hwnd,MainWndProcStub,0);

		return 0;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void TaskbarThumbnails::SetupJumplistTasks()
{
	TCHAR szCurrentProcess[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(),szCurrentProcess,SIZEOF_ARRAY(szCurrentProcess));

	TCHAR szName[256];
	LoadString(m_instance,IDS_TASKS_NEWTAB,szName,SIZEOF_ARRAY(szName));

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

ATOM TaskbarThumbnails::RegisterTabProxyClass(const TCHAR *szClassName)
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
void TaskbarThumbnails::CreateTabProxy(int iTabId,BOOL bSwitchToNewTab)
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

	if(!m_enabled)
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

		ptp->taskbarThumbnails = this;
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

void TaskbarThumbnails::RemoveTabProxy(int iTabId)
{
	if(m_bTaskbarInitialised)
	{
		for(auto itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
		{
			if(itr->iTabId == iTabId)
			{
				m_pTaskbarList->UnregisterTab(itr->hProxy);

				HICON hIcon = reinterpret_cast<HICON>(GetClassLongPtr(itr->hProxy, GCLP_HICONSM));
				DestroyIcon(hIcon);

				TabProxy_t *ptp = reinterpret_cast<TabProxy_t *>(GetWindowLongPtr(itr->hProxy,GWLP_USERDATA));
				DestroyWindow(itr->hProxy);
				free(ptp);

				UnregisterClass(reinterpret_cast<LPCWSTR>(MAKEWORD(itr->atomClass, 0)), GetModuleHandle(0));

				m_TabProxyList.erase(itr);
				break;
			}
		}
	}
}

void TaskbarThumbnails::InvalidateTaskbarThumbnailBitmap(const Tab &tab)
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
				if(itr->iTabId == tab.GetId())
				{
					DwmInvalidateIconicBitmaps(itr->hProxy);
					break;
				}
			}
		}

		FreeLibrary(hDwmapi);
	}
}

void TaskbarThumbnails::RegisterTab(HWND hTabProxy, const TCHAR *szDisplayName, BOOL bTabActive)
{
	/* Register and insert the tab into the current list of
	taskbar thumbnails. */
	m_pTaskbarList->RegisterTab(hTabProxy, m_expp->GetMainWindow());
	m_pTaskbarList->SetTabOrder(hTabProxy,NULL);

	m_pTaskbarList->SetThumbnailTooltip(hTabProxy,szDisplayName);

	if(bTabActive)
	{
		m_pTaskbarList->SetTabActive(hTabProxy, m_expp->GetMainWindow(),0);
	}
}

LRESULT CALLBACK TaskbarThumbnails::TabProxyWndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
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
		return ptp->taskbarThumbnails->TabProxyWndProc(hwnd,Msg,wParam,lParam,ptp->iTabId);
	else
		return DefWindowProc(hwnd,Msg,wParam,lParam);
}

LRESULT CALLBACK TaskbarThumbnails::TabProxyWndProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam,int iTabId)
{
	switch(Msg)
	{
	case WM_ACTIVATE:
		/* Restore the main window if necessary, and switch
		to the actual tab. */
		if(IsIconic(m_expp->GetMainWindow()))
		{
			ShowWindow(m_expp->GetMainWindow(),SW_RESTORE);
		}

		m_tabContainer->SelectTab(m_tabContainer->GetTab(iTabId));
		return 0;
		break;

	case WM_SETFOCUS:
		SetFocus(m_tabContainer->GetTab(iTabId).listView);
		break;

	case WM_SYSCOMMAND:
		switch(wParam)
		{
		case SC_CLOSE:
			break;

		default:
			SendMessage(m_tabContainer->GetTab(iTabId).listView,WM_SYSCOMMAND,wParam,lParam);
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
			if(IsIconic(m_expp->GetMainWindow()))
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

			hdc = GetDC(m_expp->GetMainWindow());
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
			ReleaseDC(m_expp->GetMainWindow(),hdc);

			return 0;
		}
		break;

	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
		{
			HMODULE hDwmapi;
			TabPreviewInfo_t tpi;

			DwmSetIconicLivePreviewBitmapProc DwmSetIconicLivePreviewBitmap;

			tpi.hbm = NULL;

			if(IsIconic(m_expp->GetMainWindow()))
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
			int nTabs = m_tabContainer->GetNumTabs();

			if(nTabs == 1)
			{
				/* If this is the last tab, we'll close
				the whole application. */
				SendMessage(m_expp->GetMainWindow(),WM_CLOSE,0,0);
			}
			else
			{
				const Tab &tab = m_tabContainer->GetTab(iTabId);
				m_tabContainer->CloseTab(tab);
			}
		}
		break;
	}

	return DefWindowProc(hwnd,Msg,wParam,lParam);
}

HBITMAP TaskbarThumbnails::CaptureTabScreenshot(int iTabId)
{
	HDC hdc;
	HDC hdcSrc;
	HBITMAP hBitmap;
	HBITMAP hPrevBitmap;
	Gdiplus::Color color(0,0,0);
	RECT rcMain;
	RECT rcTab;

	HWND hTab = m_tabContainer->GetTab(iTabId).listView;

	GetClientRect(m_expp->GetMainWindow(),&rcMain);
	GetClientRect(hTab,&rcTab);

	/* Main window BitBlt. */
	hdc = GetDC(m_expp->GetMainWindow());
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

	MapWindowPoints(hTab, m_expp->GetMainWindow(),(LPPOINT)&rcTab,2);
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
	ReleaseDC(m_expp->GetMainWindow(),hdc);

	return hbmThumbnail;
}

/* It is up to the caller to delete the bitmap returned by this method. */
void TaskbarThumbnails::GetTabLivePreviewBitmap(int iTabId,TabPreviewInfo_t *ptpi)
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

	HWND hTab = m_tabContainer->GetTab(iTabId).listView;

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

	MapWindowPoints(hTab, m_expp->GetMainWindow(),(LPPOINT)&rcTab,2);

	mbi.cbSize	 = sizeof(mbi);
	GetMenuBarInfo(m_expp->GetMainWindow(),OBJID_MENU,0,&mbi);

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

void TaskbarThumbnails::OnTabSelectionChanged(const Tab &tab)
{
	if (!m_bTaskbarInitialised)
	{
		return;
	}

	for (const TabProxyInfo_t &tabProxyInfo : m_TabProxyList)
	{
		if (tabProxyInfo.iTabId == tab.GetId())
		{
			int index = m_tabContainer->GetTabIndex(tab);

			int nTabs = m_tabContainer->GetNumTabs();

			/* Potentially the tab may have swapped position, so
			tell the taskbar to reposition it. */
			if (index == (nTabs - 1))
			{
				m_pTaskbarList->SetTabOrder(tabProxyInfo.hProxy, NULL);
			}
			else
			{
				const Tab &nextTab = m_tabContainer->GetTabByIndex(index + 1);

				for (const TabProxyInfo_t &tabProxyInfoNext : m_TabProxyList)
				{
					if (tabProxyInfoNext.iTabId == nextTab.GetId())
					{
						m_pTaskbarList->SetTabOrder(tabProxyInfo.hProxy, tabProxyInfoNext.hProxy);
						break;
					}
				}
			}

			m_pTaskbarList->SetTabActive(tabProxyInfo.hProxy, m_expp->GetMainWindow(), 0);
			break;
		}
	}
}

void TaskbarThumbnails::OnNavigationCompleted(const Tab &tab)
{
	InvalidateTaskbarThumbnailBitmap(tab);
	SetTabProxyIcon(tab);
	UpdateTaskbarThumbnailTtitle(tab);
}

void TaskbarThumbnails::UpdateTaskbarThumbnailTtitle(const Tab &tab)
{
	if (!m_bTaskbarInitialised)
	{
		return;
	}

	for (const TabProxyInfo_t &tabProxyInfo : m_TabProxyList)
	{
		if (tabProxyInfo.iTabId == tab.GetId())
		{
			SetWindowText(tabProxyInfo.hProxy, tab.GetName().c_str());
			m_pTaskbarList->SetThumbnailTooltip(tabProxyInfo.hProxy, tab.GetName().c_str());
			break;
		}
	}
}

void TaskbarThumbnails::SetTabProxyIcon(const Tab &tab)
{
	for (const TabProxyInfo_t &tabProxyInfo : m_TabProxyList)
	{
		if (tabProxyInfo.iTabId == tab.GetId())
		{
			PIDLPointer pidlDirectory(tab.GetShellBrowser()->QueryCurrentDirectoryIdl());

			/* TODO: The proxy icon may also be the lock icon, if
			the tab is locked. */
			SHFILEINFO shfi;
			DWORD_PTR res = SHGetFileInfo((LPCTSTR)pidlDirectory.get(), 0, &shfi, sizeof(shfi),
				SHGFI_PIDL | SHGFI_ICON | SHGFI_SMALLICON);

			if (!res)
			{
				return;
			}

			HICON hIconTemp = (HICON)GetClassLongPtr(tabProxyInfo.hProxy, GCLP_HICONSM);
			DestroyIcon(hIconTemp);

			hIconTemp = CopyIcon(shfi.hIcon);
			DestroyIcon(shfi.hIcon);

			SetClassLongPtr(tabProxyInfo.hProxy, GCLP_HICONSM, (LONG_PTR)hIconTemp);
			break;
		}
	}
}