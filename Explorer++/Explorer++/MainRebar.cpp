// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "ApplicationToolbar.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "Config.h"
#include "DarkModeHelper.h"
#include "DrivesToolbar.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowHelper.h"

LRESULT CALLBACK RebarSubclassStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

static const int TOOLBAR_BOOKMARK_START = 46000;
static const int TOOLBAR_BOOKMARK_END = TOOLBAR_BOOKMARK_START + 1000;
static const int TOOLBAR_DRIVES_ID_START = TOOLBAR_BOOKMARK_END + 1;
static const int TOOLBAR_DRIVES_ID_END = TOOLBAR_DRIVES_ID_START + 1000;
static const int TOOLBAR_APPLICATIONS_ID_START = TOOLBAR_DRIVES_ID_END + 1;
static const int TOOLBAR_APPLICATIONS_ID_END = TOOLBAR_APPLICATIONS_ID_START + 1000;

DWORD BookmarkToolbarStyles = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
	| TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER
	| CCS_NORESIZE;

DWORD RebarStyles = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER
	| CCS_NODIVIDER | CCS_TOP | CCS_NOPARENTALIGN | RBS_BANDBORDERS | RBS_VARHEIGHT;

void Explorerplusplus::InitializeMainToolbars()
{
	/* Initialize the main toolbar styles and settings here. The visibility and gripper
	styles will be set after the settings have been loaded (needed to keep compatibility
	with versions older than 0.9.5.4). */
	m_ToolbarInformation[0].wID = ID_MAINTOOLBAR;
	m_ToolbarInformation[0].fMask =
		RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_STYLE;
	m_ToolbarInformation[0].fStyle = RBBS_BREAK | RBBS_USECHEVRON;
	m_ToolbarInformation[0].cx = 0;
	m_ToolbarInformation[0].cxIdeal = 0;
	m_ToolbarInformation[0].cxMinChild = 0;
	m_ToolbarInformation[0].cyIntegral = 0;
	m_ToolbarInformation[0].cxHeader = 0;
	m_ToolbarInformation[0].lpText = nullptr;

	m_ToolbarInformation[1].wID = ID_ADDRESSTOOLBAR;
	m_ToolbarInformation[1].fMask =
		RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
	m_ToolbarInformation[1].fStyle = RBBS_BREAK;
	m_ToolbarInformation[1].cx = 0;
	m_ToolbarInformation[1].cxIdeal = 0;
	m_ToolbarInformation[1].cxMinChild = 0;
	m_ToolbarInformation[1].cyIntegral = 0;
	m_ToolbarInformation[1].cxHeader = 0;
	m_ToolbarInformation[1].lpText = nullptr;

	m_ToolbarInformation[2].wID = ID_BOOKMARKSTOOLBAR;
	m_ToolbarInformation[2].fMask =
		RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_STYLE;
	m_ToolbarInformation[2].fStyle = RBBS_BREAK | RBBS_USECHEVRON;
	m_ToolbarInformation[2].cx = 0;
	m_ToolbarInformation[2].cxIdeal = 0;
	m_ToolbarInformation[2].cxMinChild = 0;
	m_ToolbarInformation[2].cyIntegral = 0;
	m_ToolbarInformation[2].cxHeader = 0;
	m_ToolbarInformation[2].lpText = nullptr;

	m_ToolbarInformation[3].wID = ID_DRIVESTOOLBAR;
	m_ToolbarInformation[3].fMask =
		RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_STYLE;
	m_ToolbarInformation[3].fStyle = RBBS_BREAK | RBBS_USECHEVRON;
	m_ToolbarInformation[3].cx = 0;
	m_ToolbarInformation[3].cxIdeal = 0;
	m_ToolbarInformation[3].cxMinChild = 0;
	m_ToolbarInformation[3].cyIntegral = 0;
	m_ToolbarInformation[3].cxHeader = 0;
	m_ToolbarInformation[3].lpText = nullptr;

	m_ToolbarInformation[4].wID = ID_APPLICATIONSTOOLBAR;
	m_ToolbarInformation[4].fMask =
		RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_STYLE;
	m_ToolbarInformation[4].fStyle = RBBS_BREAK | RBBS_USECHEVRON;
	m_ToolbarInformation[4].cx = 0;
	m_ToolbarInformation[4].cxIdeal = 0;
	m_ToolbarInformation[4].cxMinChild = 0;
	m_ToolbarInformation[4].cyIntegral = 0;
	m_ToolbarInformation[4].cxHeader = 0;
	m_ToolbarInformation[4].lpText = nullptr;
}

void Explorerplusplus::CreateMainControls()
{
	SIZE sz;
	RECT rc;
	DWORD toolbarSize;
	int i = 0;

	/* If the rebar is locked, prevent bands from
	been rearranged. */
	if (m_config->lockToolbars)
	{
		RebarStyles |= RBS_FIXEDORDER;
	}

	/* Create and subclass the main rebar control. */
	m_hMainRebar = CreateWindowEx(0, REBARCLASSNAME, EMPTY_STRING, RebarStyles, 0, 0, 0, 0,
		m_hContainer, nullptr, GetModuleHandle(nullptr), nullptr);
	SetWindowSubclass(m_hMainRebar, RebarSubclassStub, 0, (DWORD_PTR) this);

	for (i = 0; i < NUM_MAIN_TOOLBARS; i++)
	{
		switch (m_ToolbarInformation[i].wID)
		{
		case ID_MAINTOOLBAR:
			CreateMainToolbar();
			toolbarSize = (DWORD) SendMessage(m_mainToolbar->GetHWND(), TB_GETBUTTONSIZE, 0, 0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(toolbarSize);
			SendMessage(m_mainToolbar->GetHWND(), TB_GETMAXSIZE, 0, (LPARAM) &sz);

			if (m_ToolbarInformation[i].cx == 0)
			{
				m_ToolbarInformation[i].cx = sz.cx;
			}

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_mainToolbar->GetHWND();
			break;

		case ID_ADDRESSTOOLBAR:
			CreateAddressBar();
			GetWindowRect(m_addressBar->GetHWND(), &rc);
			m_ToolbarInformation[i].cyMinChild = GetRectHeight(&rc);
			m_ToolbarInformation[i].hwndChild = m_addressBar->GetHWND();
			break;

		case ID_BOOKMARKSTOOLBAR:
			CreateBookmarksToolbar();
			toolbarSize = (DWORD) SendMessage(m_hBookmarksToolbar, TB_GETBUTTONSIZE, 0, 0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(toolbarSize);
			SendMessage(m_hBookmarksToolbar, TB_GETMAXSIZE, 0, (LPARAM) &sz);

			if (m_ToolbarInformation[i].cx == 0)
			{
				m_ToolbarInformation[i].cx = sz.cx;
			}

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_hBookmarksToolbar;
			break;

		case ID_DRIVESTOOLBAR:
			CreateDrivesToolbar();
			toolbarSize = (DWORD) SendMessage(m_pDrivesToolbar->GetHWND(), TB_GETBUTTONSIZE, 0, 0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(toolbarSize);
			SendMessage(m_pDrivesToolbar->GetHWND(), TB_GETMAXSIZE, 0, (LPARAM) &sz);

			if (m_ToolbarInformation[i].cx == 0)
			{
				m_ToolbarInformation[i].cx = sz.cx;
			}

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_pDrivesToolbar->GetHWND();
			break;

		case ID_APPLICATIONSTOOLBAR:
			CreateApplicationToolbar();
			toolbarSize =
				(DWORD) SendMessage(m_pApplicationToolbar->GetHWND(), TB_GETBUTTONSIZE, 0, 0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(toolbarSize);
			SendMessage(m_pApplicationToolbar->GetHWND(), TB_GETMAXSIZE, 0, (LPARAM) &sz);

			if (m_ToolbarInformation[i].cx == 0)
			{
				m_ToolbarInformation[i].cx = sz.cx;
			}

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_pApplicationToolbar->GetHWND();
			break;
		}

		m_ToolbarInformation[i].cbSize = sizeof(REBARBANDINFO);
		SendMessage(m_hMainRebar, RB_INSERTBAND, (WPARAM) -1, (LPARAM) &m_ToolbarInformation[i]);
	}
}

LRESULT CALLBACK RebarSubclassStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pContainer = (Explorerplusplus *) dwRefData;

	return pContainer->RebarSubclass(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::RebarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITMENU:
		SendMessage(m_hContainer, WM_INITMENU, wParam, lParam);
		break;

	case WM_MENUSELECT:
		SendMessage(m_hContainer, WM_MENUSELECT, wParam, lParam);
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code)
		{
		case NM_RCLICK:
		{
			auto pnmm = reinterpret_cast<LPNMMOUSE>(lParam);
			OnToolbarRClick(pnmm->hdr.hwndFrom);
		}
			return TRUE;

		case NM_CUSTOMDRAW:
			if (auto result = OnRebarCustomDraw(reinterpret_cast<NMHDR *>(lParam)))
			{
				return *result;
			}
			break;
		}
		break;

	case WM_ERASEBKGND:
		if (OnRebarEraseBackground(reinterpret_cast<HDC>(wParam)))
		{
			return 1;
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void Explorerplusplus::OnToolbarRClick(HWND sourceWindow)
{
	auto parentMenu =
		wil::unique_hmenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_TOOLBAR_MENU)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	MenuHelper::CheckItem(menu, IDM_TOOLBARS_ADDRESSBAR, m_config->showAddressBar);
	MenuHelper::CheckItem(menu, IDM_TOOLBARS_MAINTOOLBAR, m_config->showMainToolbar);
	MenuHelper::CheckItem(menu, IDM_TOOLBARS_BOOKMARKSTOOLBAR, m_config->showBookmarksToolbar);
	MenuHelper::CheckItem(menu, IDM_TOOLBARS_DRIVES, m_config->showDrivesToolbar);
	MenuHelper::CheckItem(menu, IDM_TOOLBARS_APPLICATIONTOOLBAR, m_config->showApplicationToolbar);
	MenuHelper::CheckItem(menu, IDM_TOOLBARS_LOCKTOOLBARS, m_config->lockToolbars);

	DWORD dwPos = GetMessagePos();

	POINT ptCursor;
	ptCursor.x = GET_X_LPARAM(dwPos);
	ptCursor.y = GET_Y_LPARAM(dwPos);

	// Give any observers a chance to modify the menu.
	m_toolbarContextMenuSignal(menu, sourceWindow, ptCursor);

	TrackPopupMenu(menu, TPM_LEFTALIGN, ptCursor.x, ptCursor.y, 0, m_hMainRebar, nullptr);
}

boost::signals2::connection Explorerplusplus::AddToolbarContextMenuObserver(
	const ToolbarContextMenuSignal::slot_type &observer)
{
	return m_toolbarContextMenuSignal.connect(observer);
}

void Explorerplusplus::CreateAddressBar()
{
	m_addressBar = AddressBar::Create(m_hMainRebar, this, m_mainToolbar);
}

void Explorerplusplus::CreateMainToolbar()
{
	m_mainToolbar = MainToolbar::Create(m_hMainRebar, m_hLanguageModule, this, m_config);

	// This should be done in the MainToolbar class. However, the TB_SAVERESTORE
	// message needs to be sent to the toolbar window. That's incompatible with
	// how the rest of the settings in the application tend to be loaded.
	// It's generally assumed that settings and data can be loaded first (early
	// in the lifetime of the application) and then the controls can be
	// initialized based on those settings.
	// Sending TB_SAVERESTORE means that the toolbar window needs to exist.
	// That's true whether data is being saved or being loaded.
	// Rather than do this inside the MainToolbar class, which would result in
	// data being loaded from the registry in a different way to how it's loaded
	// in other classes, the message is simply sent here for now.
	// Ultimately, it would likely be better not to use this message, especially
	// since it can't be used to save data to anything but the registry.
	if (!m_bLoadSettingsFromXML)
	{
		if (m_bAttemptToolbarRestore)
		{
			TBSAVEPARAMS tbSave;
			tbSave.hkr = HKEY_CURRENT_USER;
			tbSave.pszSubKey = NExplorerplusplus::REG_SETTINGS_KEY;
			tbSave.pszValueName = _T("ToolbarState");
			SendMessage(
				m_mainToolbar->GetHWND(), TB_SAVERESTORE, FALSE, reinterpret_cast<LPARAM>(&tbSave));

			// As part of restoring the toolbar, the state of some items may be
			// lost, so set their state again here.
			m_mainToolbar->UpdateConfigDependentButtonStates();
		}
	}

	// The main toolbar will update its size when the useLargeToolbarIcons
	// option changes. The rebar also needs to be updated, though only after the
	// toolbar has updated itself. It's possible this would be better done by
	// having the main toolbar send out a custom event once it's updated its own
	// size, rather than relying on the observer here running after the one set
	// up by the main toolbar.
	m_connections.push_back(m_config->useLargeToolbarIcons.addObserver(
		boost::bind(&Explorerplusplus::OnUseLargeToolbarIconsUpdated, this, _1),
		boost::signals2::at_back));
}

void Explorerplusplus::CreateBookmarksToolbar()
{
	m_hBookmarksToolbar = CreateToolbar(m_hMainRebar, BookmarkToolbarStyles,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_DOUBLEBUFFER
			| TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	m_pBookmarksToolbar =
		new BookmarksToolbar(m_hBookmarksToolbar, m_hLanguageModule, this, m_navigation.get(),
			&m_bookmarkIconFetcher, &m_bookmarkTree, TOOLBAR_BOOKMARK_START, TOOLBAR_BOOKMARK_END);
}

void Explorerplusplus::CreateDrivesToolbar()
{
	m_pDrivesToolbar = DrivesToolbar::Create(m_hMainRebar, TOOLBAR_DRIVES_ID_START,
		TOOLBAR_DRIVES_ID_END, m_hLanguageModule, this, m_navigation.get());
}

void Explorerplusplus::CreateApplicationToolbar()
{
	m_pApplicationToolbar = ApplicationToolbar::Create(m_hMainRebar, TOOLBAR_APPLICATIONS_ID_START,
		TOOLBAR_APPLICATIONS_ID_END, m_hLanguageModule, this);
}

void Explorerplusplus::OnUseLargeToolbarIconsUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	auto buttonSize =
		static_cast<DWORD>(SendMessage(m_mainToolbar->GetHWND(), TB_GETBUTTONSIZE, 0, 0));

	REBARBANDINFO bandInfo;
	bandInfo.cbSize = sizeof(bandInfo);
	bandInfo.fMask = RBBIM_CHILDSIZE;
	bandInfo.cxMinChild = 0;
	bandInfo.cyMinChild = HIWORD(buttonSize);
	bandInfo.cyChild = HIWORD(buttonSize);
	bandInfo.cyMaxChild = HIWORD(buttonSize);
	SendMessage(m_hMainRebar, RB_SETBANDINFO, 0, reinterpret_cast<LPARAM>(&bandInfo));
}

HMENU Explorerplusplus::CreateRebarHistoryMenu(BOOL bBack)
{
	HMENU hSubMenu = nullptr;
	std::vector<HistoryEntry *> history;
	int iBase;

	const Tab &tab = m_tabContainer->GetSelectedTab();

	if (bBack)
	{
		iBase = ID_REBAR_MENU_BACK_START;
		history = tab.GetShellBrowser()->GetNavigationController()->GetBackHistory();
	}
	else
	{
		iBase = ID_REBAR_MENU_FORWARD_START;
		history = tab.GetShellBrowser()->GetNavigationController()->GetForwardHistory();
	}

	if (!history.empty())
	{
		int i = 0;

		hSubMenu = CreateMenu();

		for (auto &entry : history)
		{
			std::wstring displayName = entry->GetDisplayName();

			MENUITEMINFO mii;
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_ID | MIIM_STRING;
			mii.wID = iBase + i + 1;
			mii.dwTypeData = displayName.data();
			InsertMenuItem(hSubMenu, i, TRUE, &mii);

			i++;
		}
	}

	return hSubMenu;
}

std::optional<int> Explorerplusplus::OnRebarCustomDraw(NMHDR *nmhdr)
{
	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return std::nullopt;
	}

	if (nmhdr->hwndFrom != m_mainToolbar->GetHWND() && nmhdr->hwndFrom != m_hBookmarksToolbar
		&& nmhdr->hwndFrom != m_pDrivesToolbar->GetHWND()
		&& nmhdr->hwndFrom != m_pApplicationToolbar->GetHWND())
	{
		return std::nullopt;
	}

	auto *customDraw = reinterpret_cast<NMTBCUSTOMDRAW *>(nmhdr);

	switch (customDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		customDraw->clrText = DarkModeHelper::TEXT_COLOR;
		customDraw->clrHighlightHotTrack = DarkModeHelper::BUTTON_HIGHLIGHT_COLOR;
		return TBCDRF_USECDCOLORS | TBCDRF_HILITEHOTTRACK;
	}

	return std::nullopt;
}

bool Explorerplusplus::OnRebarEraseBackground(HDC hdc)
{
	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return false;
	}

	RECT rc;
	GetClientRect(m_hMainRebar, &rc);
	FillRect(hdc, &rc, darkModeHelper.GetBackgroundBrush());

	return true;
}