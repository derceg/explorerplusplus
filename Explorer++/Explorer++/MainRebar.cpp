// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "ApplicationModelFactory.h"
#include "ApplicationToolbar.h"
#include "ApplicationToolbarView.h"
#include "Bookmarks/BookmarkTreeFactory.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "Bookmarks/UI/Views/BookmarksToolbarView.h"
#include "Config.h"
#include "DriveEnumeratorImpl.h"
#include "DriveModel.h"
#include "DriveWatcherImpl.h"
#include "DrivesToolbar.h"
#include "DrivesToolbarView.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowHelper.h"

LRESULT CALLBACK RebarSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

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
	m_hMainRebar = CreateWindowEx(WS_EX_CONTROLPARENT, REBARCLASSNAME, EMPTY_STRING, RebarStyles, 0,
		0, 0, 0, m_hContainer, nullptr, GetModuleHandle(nullptr), nullptr);
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
			toolbarSize = (DWORD) SendMessage(m_bookmarksToolbar->GetView()->GetHWND(),
				TB_GETBUTTONSIZE, 0, 0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(toolbarSize);
			SendMessage(m_bookmarksToolbar->GetView()->GetHWND(), TB_GETMAXSIZE, 0, (LPARAM) &sz);

			if (m_ToolbarInformation[i].cx == 0)
			{
				m_ToolbarInformation[i].cx = sz.cx;
			}

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_bookmarksToolbar->GetView()->GetHWND();
			break;

		case ID_DRIVESTOOLBAR:
			CreateDrivesToolbar();
			toolbarSize =
				(DWORD) SendMessage(m_drivesToolbar->GetView()->GetHWND(), TB_GETBUTTONSIZE, 0, 0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(toolbarSize);
			SendMessage(m_drivesToolbar->GetView()->GetHWND(), TB_GETMAXSIZE, 0, (LPARAM) &sz);

			if (m_ToolbarInformation[i].cx == 0)
			{
				m_ToolbarInformation[i].cx = sz.cx;
			}

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_drivesToolbar->GetView()->GetHWND();
			break;

		case ID_APPLICATIONSTOOLBAR:
			CreateApplicationToolbar();
			toolbarSize = (DWORD) SendMessage(m_applicationToolbar->GetView()->GetHWND(),
				TB_GETBUTTONSIZE, 0, 0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(toolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(toolbarSize);
			SendMessage(m_applicationToolbar->GetView()->GetHWND(), TB_GETMAXSIZE, 0, (LPARAM) &sz);

			if (m_ToolbarInformation[i].cx == 0)
			{
				m_ToolbarInformation[i].cx = sz.cx;
			}

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_applicationToolbar->GetView()->GetHWND();
			break;
		}

		m_ToolbarInformation[i].cbSize = sizeof(REBARBANDINFO);
		SendMessage(m_hMainRebar, RB_INSERTBAND, static_cast<WPARAM>(-1),
			(LPARAM) &m_ToolbarInformation[i]);
	}
}

LRESULT CALLBACK RebarSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
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
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void Explorerplusplus::OnToolbarRClick(HWND sourceWindow)
{
	auto parentMenu =
		wil::unique_hmenu(LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_TOOLBAR_MENU)));

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

	int menuItemId = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RETURNCMD, ptCursor.x, ptCursor.y, 0,
		m_hMainRebar, nullptr);

	if (menuItemId == 0)
	{
		return;
	}

	OnToolbarMenuItemSelected(sourceWindow, menuItemId);
}

void Explorerplusplus::OnToolbarMenuItemSelected(HWND sourceWindow, int menuItemId)
{
	switch (menuItemId)
	{
	case IDM_TOOLBARS_ADDRESSBAR:
		OnToggleAddressBar();
		break;

	case IDM_TOOLBARS_MAINTOOLBAR:
		OnToggleMainToolbar();
		break;

	case IDM_TOOLBARS_BOOKMARKSTOOLBAR:
		OnToggleBookmarksToolbar();
		break;

	case IDM_TOOLBARS_DRIVES:
		OnToggleDrivesToolbar();
		break;

	case IDM_TOOLBARS_APPLICATIONTOOLBAR:
		OnToggleApplicationToolbar();
		break;

	case IDM_TOOLBARS_LOCKTOOLBARS:
		OnLockToolbars();
		break;

	case IDM_TOOLBARS_CUSTOMIZE:
		OnCustomizeMainToolbar();
		break;

	default:
		m_toolbarContextMenuSelectedSignal(sourceWindow, menuItemId);
		break;
	}
}

void Explorerplusplus::OnToggleAddressBar()
{
	m_config->showAddressBar = !m_config->showAddressBar;
	OnToggleToolbar(m_addressBar->GetHWND(), m_config->showAddressBar);
}

void Explorerplusplus::OnToggleMainToolbar()
{
	m_config->showMainToolbar = !m_config->showMainToolbar;
	OnToggleToolbar(m_mainToolbar->GetHWND(), m_config->showMainToolbar);
}

void Explorerplusplus::OnToggleBookmarksToolbar()
{
	m_config->showBookmarksToolbar = !m_config->showBookmarksToolbar;
	OnToggleToolbar(m_bookmarksToolbar->GetView()->GetHWND(), m_config->showBookmarksToolbar);
}

void Explorerplusplus::OnToggleDrivesToolbar()
{
	m_config->showDrivesToolbar = !m_config->showDrivesToolbar;
	OnToggleToolbar(m_drivesToolbar->GetView()->GetHWND(), m_config->showDrivesToolbar);
}

void Explorerplusplus::OnToggleApplicationToolbar()
{
	m_config->showApplicationToolbar = !m_config->showApplicationToolbar;
	OnToggleToolbar(m_applicationToolbar->GetView()->GetHWND(), m_config->showApplicationToolbar);
}

void Explorerplusplus::OnToggleToolbar(HWND toolbar, bool show)
{
	ShowMainRebarBand(toolbar, show);
}

void Explorerplusplus::OnCustomizeMainToolbar()
{
	SendMessage(m_mainToolbar->GetHWND(), TB_CUSTOMIZE, 0, 0);
}

boost::signals2::connection Explorerplusplus::AddToolbarContextMenuObserver(
	const ToolbarContextMenuSignal::slot_type &observer)
{
	return m_toolbarContextMenuSignal.connect(observer);
}

boost::signals2::connection Explorerplusplus::AddToolbarContextMenuSelectedObserver(
	const ToolbarContextMenuSelectedSignal::slot_type &observer)
{
	return m_toolbarContextMenuSelectedSignal.connect(observer);
}

void Explorerplusplus::CreateAddressBar()
{
	m_addressBar = AddressBar::Create(m_hMainRebar, this, this);
}

void Explorerplusplus::CreateMainToolbar()
{
	m_mainToolbar = MainToolbar::Create(m_hMainRebar, m_resourceInstance, this, m_config);

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
			SendMessage(m_mainToolbar->GetHWND(), TB_SAVERESTORE, FALSE,
				reinterpret_cast<LPARAM>(&tbSave));

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
		std::bind_front(&Explorerplusplus::OnUseLargeToolbarIconsUpdated, this),
		boost::signals2::at_back));
}

void Explorerplusplus::CreateBookmarksToolbar()
{
	auto bookmarksToolbarView = new BookmarksToolbarView(m_hMainRebar);

	m_bookmarksToolbar = BookmarksToolbar::Create(bookmarksToolbarView, this, this,
		&m_bookmarkIconFetcher, BookmarkTreeFactory::GetInstance()->GetBookmarkTree());
	m_bookmarksToolbar->GetView()->AddToolbarUpdatedObserver(std::bind_front(
		&Explorerplusplus::OnRebarToolbarUpdated, this, m_bookmarksToolbar->GetView()->GetHWND()));
}

void Explorerplusplus::CreateDrivesToolbar()
{
	auto drivesToolbarView = DrivesToolbarView::Create(m_hMainRebar);

	auto driveEnumerator = std::make_unique<DriveEnumeratorImpl>();
	auto driveWatcher = std::make_unique<DriveWatcherImpl>(m_hContainer);
	auto driveModel =
		std::make_unique<DriveModel>(std::move(driveEnumerator), std::move(driveWatcher));

	m_drivesToolbar = DrivesToolbar::Create(drivesToolbarView, std::move(driveModel), this, this);
	m_drivesToolbar->GetView()->AddToolbarUpdatedObserver(std::bind_front(
		&Explorerplusplus::OnRebarToolbarUpdated, this, m_drivesToolbar->GetView()->GetHWND()));
}

void Explorerplusplus::CreateApplicationToolbar()
{
	auto applicationToolbarView = Applications::ApplicationToolbarView::Create(m_hMainRebar);

	m_applicationToolbar = Applications::ApplicationToolbar::Create(applicationToolbarView,
		Applications::ApplicationModelFactory::GetInstance()->GetApplicationModel(), this);
	m_applicationToolbar->GetView()->AddToolbarUpdatedObserver(
		std::bind_front(&Explorerplusplus::OnRebarToolbarUpdated, this,
			m_applicationToolbar->GetView()->GetHWND()));
}

void Explorerplusplus::OnRebarToolbarUpdated(HWND toolbar)
{
	UpdateToolbarBandSizing(m_hMainRebar, toolbar);
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
