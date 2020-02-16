// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * The methods in this file manage the tab proxy windows. A tab
 * proxy is created when a taskbar thumbnail needs to be shown.
 */

#include "stdafx.h"
#include "TaskbarThumbnails.h"
#include "Config.h"
#include "CoreInterface.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabContainer.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include <dwmapi.h>

namespace
{
	struct TabProxy_t
	{
		TaskbarThumbnails *taskbarThumbnails;
		int iTabId;
	};
}

TaskbarThumbnails *TaskbarThumbnails::Create(IExplorerplusplus *expp, TabContainer *tabContainer,
	HINSTANCE instance, std::shared_ptr<Config> config)
{
	return new TaskbarThumbnails(expp, tabContainer, instance, config);
}

TaskbarThumbnails::TaskbarThumbnails(IExplorerplusplus *expp, TabContainer *tabContainer,
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

void TaskbarThumbnails::Initialize()
{
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

	m_tabContainer->tabCreatedSignal.AddObserver(boost::bind(&TaskbarThumbnails::CreateTabProxy, this, _1, _2));
	m_tabContainer->tabNavigationCompletedSignal.AddObserver(boost::bind(&TaskbarThumbnails::OnNavigationCompleted, this, _1));
	m_tabContainer->tabSelectedSignal.AddObserver(boost::bind(&TaskbarThumbnails::OnTabSelectionChanged, this, _1));
	m_tabContainer->tabRemovedSignal.AddObserver(boost::bind(&TaskbarThumbnails::RemoveTabProxy, this, _1));
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
		if(m_pTaskbarList != nullptr)
		{
			m_pTaskbarList->Release();
		}

		CoCreateInstance(CLSID_TaskbarList, nullptr,CLSCTX_INPROC_SERVER,
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

			UpdateTaskbarThumbnailTitle(tab);
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
	wcex.hInstance		= GetModuleHandle(nullptr);
	wcex.hIcon			= nullptr;
	wcex.hIconSm		= nullptr;
	wcex.hCursor		= LoadCursor(nullptr,IDC_ARROW);
	wcex.hbrBackground	= nullptr;
	wcex.lpszMenuName	= nullptr;
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

	if(!m_enabled)
	{
		return;
	}

	static int iCount = 0;

	StringCchPrintf(szClassName,SIZEOF_ARRAY(szClassName),_T("Explorer++TabProxy%d"),iCount++);

	aRet = RegisterTabProxyClass(szClassName);

	if(aRet != 0)
	{
		TabProxy_t *ptp = nullptr;

		ptp = (TabProxy_t *)malloc(sizeof(TabProxy_t));

		ptp->taskbarThumbnails = this;
		ptp->iTabId = iTabId;

		hTabProxy = CreateWindow(szClassName,EMPTY_STRING,WS_OVERLAPPEDWINDOW,
			0,0,0,0,nullptr,nullptr,GetModuleHandle(nullptr),(LPVOID)ptp);

		if(hTabProxy != nullptr)
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

			m_TabProxyList.push_back(std::move(tpi));

			const Tab &tab = m_tabContainer->GetTab(iTabId);
			SetTabProxyIcon(tab);
			UpdateTaskbarThumbnailTitle(tab);
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

				TabProxy_t *ptp = reinterpret_cast<TabProxy_t *>(GetWindowLongPtr(itr->hProxy,GWLP_USERDATA));
				DestroyWindow(itr->hProxy);
				free(ptp);

				UnregisterClass(reinterpret_cast<LPCWSTR>(MAKEWORD(itr->atomClass, 0)), GetModuleHandle(nullptr));

				m_TabProxyList.erase(itr);
				break;
			}
		}
	}
}

void TaskbarThumbnails::InvalidateTaskbarThumbnailBitmap(const Tab &tab)
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

void TaskbarThumbnails::RegisterTab(HWND hTabProxy, const TCHAR *szDisplayName, BOOL bTabActive)
{
	/* Register and insert the tab into the current list of
	taskbar thumbnails. */
	m_pTaskbarList->RegisterTab(hTabProxy, m_expp->GetMainWindow());
	m_pTaskbarList->SetTabOrder(hTabProxy, nullptr);

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

	if(ptp != nullptr)
		return ptp->taskbarThumbnails->TabProxyWndProc(hwnd,Msg,wParam,lParam,ptp->iTabId);
	else
		return DefWindowProc(hwnd,Msg,wParam,lParam);
}

LRESULT CALLBACK TaskbarThumbnails::TabProxyWndProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam,int iTabId)
{
	const Tab *tab = m_tabContainer->GetTabOptional(iTabId);

	switch(Msg)
	{
	case WM_ACTIVATE:
		/* Restore the main window if necessary, and switch
		to the actual tab. */
		if(IsIconic(m_expp->GetMainWindow()))
		{
			ShowWindow(m_expp->GetMainWindow(),SW_RESTORE);
		}

		m_tabContainer->SelectTab(*tab);
		return 0;

	case WM_SETFOCUS:
		SetFocus(tab->GetShellBrowser()->GetListView());
		break;

	case WM_SYSCOMMAND:
		switch(wParam)
		{
		case SC_CLOSE:
			break;

		default:
			SendMessage(tab->GetShellBrowser()->GetListView(),WM_SYSCOMMAND,wParam,lParam);
			break;
		}
		break;

	/* Generate a thumbnail of the current tab. Basic procedure:
	1. Generate a full-scale bitmap of the main window.
	2. Overlay a bitmap of the specified tab onto the main
	window bitmap.
	3. Shrink the resulting bitmap down to the correct thumbnail size.
	
	A thumbnail will be dynamically generated, provided the main window
	is not currently minimized (as we won't be able to grab a screenshot
	of it). If the main window is minimized, we'll use a cached screenshot
	of the tab (taken before the main window was minimized). */
	case WM_DWMSENDICONICTHUMBNAIL:
		OnDwmSendIconicThumbnail(hwnd, *tab, HIWORD(lParam), LOWORD(lParam));
		return 0;

	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
		{
			if(IsIconic(m_expp->GetMainWindow()))
			{
				/* TODO: Show an image here... */
			}
			else
			{
				wil::unique_hbitmap bitmap = GetTabLivePreviewBitmap(*tab);

				RECT rcTab;
				GetClientRect(tab->GetShellBrowser()->GetListView(), &rcTab);
				MapWindowPoints(tab->GetShellBrowser()->GetListView(), m_expp->GetMainWindow(), reinterpret_cast<LPPOINT>(&rcTab), 2);

				MENUBARINFO mbi;
				mbi.cbSize = sizeof(mbi);
				GetMenuBarInfo(m_expp->GetMainWindow(), OBJID_MENU, 0, &mbi);

				POINT ptOrigin;

				/* The operating system will automatically draw the main window. Therefore,
				we'll just shift the tab into it's proper position. */
				ptOrigin.x = rcTab.left;

				/* Need to include the menu bar in the offset. */
				ptOrigin.y = rcTab.top + mbi.rcBar.bottom - mbi.rcBar.top;

				DwmSetIconicLivePreviewBitmap(hwnd, bitmap.get(), &ptOrigin, 0);
			}

			return 0;
		}

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
				m_tabContainer->CloseTab(*tab);
			}
		}
		break;
	}

	return DefWindowProc(hwnd,Msg,wParam,lParam);
}

void TaskbarThumbnails::OnDwmSendIconicThumbnail(HWND tabProxy, const Tab &tab, int maxWidth, int maxHeight)
{
	wil::unique_hbitmap hbmTab;

	/* If the main window is minimized, it won't be possible
	to generate a thumbnail for any of the tabs. In that
	case, use a static 'No Preview Available' bitmap. */
	if (IsIconic(m_expp->GetMainWindow()))
	{
		hbmTab.reset(static_cast<HBITMAP>(LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDB_NOPREVIEWAVAILABLE), IMAGE_BITMAP, 0, 0, 0)));

		SetBitmapDimensionEx(hbmTab.get(), 223, 130, nullptr);
	}
	else
	{
		hbmTab = CaptureTabScreenshot(tab);
	}

	SIZE currentSize;
	GetBitmapDimensionEx(hbmTab.get(), &currentSize);


	/* Shrink the bitmap. */
	wil::unique_hdc_window hdc = wil::GetDC(m_expp->GetMainWindow());
	wil::unique_hdc hdcSrc(CreateCompatibleDC(hdc.get()));

	auto previousTabBitmap = wil::SelectObject(hdcSrc.get(), hbmTab.get());

	wil::unique_hdc hdcThumbnailSrc(CreateCompatibleDC(hdc.get()));

	int finalWidth;
	int finalHeight;

	/* If the current height of the main window
	is less than the width, we'll create a thumbnail
	of maximum width; else maximum height. */
	if (((double)currentSize.cx / (double)maxWidth) > ((double)currentSize.cy / (double)maxHeight))
	{
		finalWidth = maxWidth;
		finalHeight = (int)ceil(maxWidth * ((double)currentSize.cy / (double)currentSize.cx));
	}
	else
	{
		finalHeight = maxHeight;
		finalWidth = (int)ceil(maxHeight * ((double)currentSize.cx / (double)currentSize.cy));
	}

	wil::unique_hbitmap hbmThumbnail;
	Gdiplus::Color color(0, 0, 0);
	Gdiplus::Bitmap bmpThumbnail(finalWidth, finalHeight, PixelFormat32bppARGB);
	bmpThumbnail.GetHBITMAP(color, &hbmThumbnail);

	auto previousThumbnailBitmap = wil::SelectObject(hdcThumbnailSrc.get(), hbmThumbnail.get());

	/* Finally, shrink the full-scale bitmap down into a thumbnail. */
	POINT pt;
	SetStretchBltMode(hdcThumbnailSrc.get(), HALFTONE);
	SetBrushOrgEx(hdcThumbnailSrc.get(), 0, 0, &pt);
	StretchBlt(hdcThumbnailSrc.get(), 0, 0, finalWidth, finalHeight, hdcSrc.get(), 0, 0, currentSize.cx, currentSize.cy, SRCCOPY);

	DwmSetIconicThumbnail(tabProxy, hbmThumbnail.get(), 0);
}

wil::unique_hbitmap TaskbarThumbnails::CaptureTabScreenshot(const Tab &tab)
{
	wil::unique_hdc_window hdc = wil::GetDC(m_expp->GetMainWindow());
	wil::unique_hdc hdcSrc(CreateCompatibleDC(hdc.get()));

	RECT rcMain;
	GetClientRect(m_expp->GetMainWindow(), &rcMain);

	/* Any bitmap sent back to the operating system will need to be in 32-bit
	ARGB format. */
	wil::unique_hbitmap hBitmap;
	Gdiplus::Color color(0, 0, 0);
	Gdiplus::Bitmap bi(GetRectWidth(&rcMain), GetRectHeight(&rcMain), PixelFormat32bppARGB);
	bi.GetHBITMAP(color, &hBitmap);

	/* Draw the main window into the bitmap. */
	auto mainWindowPreviousBitmap = wil::SelectObject(hdcSrc.get(), hBitmap.get());
	BitBlt(hdcSrc.get(), 0, 0, GetRectWidth(&rcMain), GetRectHeight(&rcMain), hdc.get(), 0, 0, SRCCOPY);


	/* Now draw the tab onto the main window. */
	RECT rcTab;
	GetClientRect(tab.GetShellBrowser()->GetListView(), &rcTab);

	wil::unique_hdc_window hdcTab = wil::GetDC(tab.GetShellBrowser()->GetListView());
	wil::unique_hdc hdcTabSrc(CreateCompatibleDC(hdcTab.get()));
	wil::unique_hbitmap hbmTab(CreateCompatibleBitmap(hdcTab.get(), GetRectWidth(&rcTab), GetRectHeight(&rcTab)));

	auto tabPreviousBitmap = wil::SelectObject(hdcTabSrc.get(), hbmTab.get());

	BOOL bVisible = IsWindowVisible(tab.GetShellBrowser()->GetListView());

	if (!bVisible)
	{
		ShowWindow(tab.GetShellBrowser()->GetListView(), SW_SHOW);
	}

	PrintWindow(tab.GetShellBrowser()->GetListView(), hdcTabSrc.get(), PW_CLIENTONLY);

	if (!bVisible)
	{
		ShowWindow(tab.GetShellBrowser()->GetListView(), SW_HIDE);
	}

	MapWindowPoints(tab.GetShellBrowser()->GetListView(), m_expp->GetMainWindow(), reinterpret_cast<LPPOINT>(&rcTab), 2);
	BitBlt(hdcSrc.get(), rcTab.left, rcTab.top, GetRectWidth(&rcTab), GetRectHeight(&rcTab), hdcTabSrc.get(), 0, 0, SRCCOPY);


	/* Shrink the bitmap. */
	wil::unique_hdc hdcThumbnailSrc(CreateCompatibleDC(hdc.get()));

	wil::unique_hbitmap hbmThumbnail;
	Gdiplus::Bitmap bmpThumbnail(GetRectWidth(&rcMain), GetRectHeight(&rcMain), PixelFormat32bppARGB);
	bmpThumbnail.GetHBITMAP(color, &hbmThumbnail);

	auto thumbnailPreviousBitmap = wil::SelectObject(hdcThumbnailSrc.get(), hbmThumbnail.get());

	/* Finally, shrink the full-scale bitmap down into a thumbnail. */
	POINT pt;
	SetStretchBltMode(hdcThumbnailSrc.get(), HALFTONE);
	SetBrushOrgEx(hdcThumbnailSrc.get(), 0, 0, &pt);
	BitBlt(hdcThumbnailSrc.get(), 0, 0, GetRectWidth(&rcMain), GetRectHeight(&rcMain), hdcSrc.get(), 0, 0, SRCCOPY);

	SetBitmapDimensionEx(hbmThumbnail.get(), GetRectWidth(&rcMain), GetRectHeight(&rcMain), nullptr);

	return hbmThumbnail;
}

wil::unique_hbitmap TaskbarThumbnails::GetTabLivePreviewBitmap(const Tab &tab)
{
	wil::unique_hdc_window hdcTab = wil::GetDC(tab.GetShellBrowser()->GetListView());
	wil::unique_hdc hdcTabSrc(CreateCompatibleDC(hdcTab.get()));

	RECT rcTab;
	GetClientRect(tab.GetShellBrowser()->GetListView(), &rcTab);

	wil::unique_hbitmap hbmTab;
	Gdiplus::Color color(0, 0, 0);
	Gdiplus::Bitmap bi(GetRectWidth(&rcTab), GetRectHeight(&rcTab), PixelFormat32bppARGB);
	bi.GetHBITMAP(color, &hbmTab);

	auto tabPreviousBitmap = wil::SelectObject(hdcTabSrc.get(), hbmTab.get());

	BOOL bVisible = IsWindowVisible(tab.GetShellBrowser()->GetListView());

	if (!bVisible)
	{
		ShowWindow(tab.GetShellBrowser()->GetListView(), SW_SHOW);
	}

	PrintWindow(tab.GetShellBrowser()->GetListView(), hdcTabSrc.get(), PW_CLIENTONLY);

	if (!bVisible)
	{
		ShowWindow(tab.GetShellBrowser()->GetListView(), SW_HIDE);
	}

	SetStretchBltMode(hdcTabSrc.get(), HALFTONE);
	SetBrushOrgEx(hdcTabSrc.get(), 0, 0, nullptr);
	StretchBlt(hdcTabSrc.get(), 0, 0, GetRectWidth(&rcTab), GetRectHeight(&rcTab), hdcTabSrc.get(),
		0, 0, GetRectWidth(&rcTab), GetRectHeight(&rcTab), SRCCOPY);

	return hbmTab;
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
				m_pTaskbarList->SetTabOrder(tabProxyInfo.hProxy, nullptr);
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
	UpdateTaskbarThumbnailTitle(tab);
}

void TaskbarThumbnails::UpdateTaskbarThumbnailTitle(const Tab &tab)
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
	for (TabProxyInfo_t &tabProxyInfo : m_TabProxyList)
	{
		if (tabProxyInfo.iTabId == tab.GetId())
		{
			auto pidlDirectory = tab.GetShellBrowser()->GetDirectoryIdl();

			/* TODO: The proxy icon may also be the lock icon, if
			the tab is locked. */
			SHFILEINFO shfi;
			DWORD_PTR res = SHGetFileInfo((LPCTSTR)pidlDirectory.get(), 0, &shfi, sizeof(shfi),
				SHGFI_PIDL | SHGFI_ICON | SHGFI_SMALLICON);

			if (!res)
			{
				return;
			}

			tabProxyInfo.icon.reset(shfi.hIcon);

			SetClassLongPtr(tabProxyInfo.hProxy, GCLP_HICONSM, (LONG_PTR)tabProxyInfo.icon.get());
			break;
		}
	}
}