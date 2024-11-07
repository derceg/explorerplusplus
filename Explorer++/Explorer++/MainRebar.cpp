// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "App.h"
#include "ApplicationToolbar.h"
#include "ApplicationToolbarView.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "Bookmarks/UI/Views/BookmarksToolbarView.h"
#include "Config.h"
#include "DriveEnumeratorImpl.h"
#include "DriveModel.h"
#include "DriveWatcherImpl.h"
#include "DrivesToolbar.h"
#include "DrivesToolbarView.h"
#include "Explorer++_internal.h"
#include "MainRebarStorage.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowHelper.h"

void Explorerplusplus::CreateMainRebarAndChildren()
{
	m_hMainRebar = CreateWindowEx(WS_EX_CONTROLPARENT, REBARCLASSNAME, L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_BORDER | CCS_NODIVIDER
			| CCS_TOP | CCS_NOPARENTALIGN | RBS_BANDBORDERS | RBS_VARHEIGHT,
		0, 0, 0, 0, m_hContainer, nullptr, GetModuleHandle(nullptr), nullptr);
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hMainRebar,
		std::bind_front(&Explorerplusplus::RebarSubclass, this)));

	auto bands = InitializeMainRebarBands();

	for (const auto &band : bands)
	{
		InsertMainRebarBand(band);
	}

	m_rebarConnections.push_back(m_config->lockToolbars.addObserver(
		std::bind_front(&Explorerplusplus::OnLockToolbarsUpdated, this)));
}

std::vector<Explorerplusplus::InternalRebarBandInfo> Explorerplusplus::InitializeMainRebarBands()
{
	std::vector<InternalRebarBandInfo> mainRebarBands;

	CreateMainToolbar();
	auto internalBandInfo = InitializeToolbarBand(REBAR_BAND_ID_MAIN_TOOLBAR,
		m_mainToolbar->GetHWND(), m_config->showMainToolbar.get());
	mainRebarBands.push_back(internalBandInfo);

	m_rebarConnections.push_back(m_config->showMainToolbar.addObserver(
		std::bind_front(&Explorerplusplus::ShowMainRebarBand, this, m_mainToolbar->GetHWND())));

	CreateAddressBar();
	internalBandInfo = InitializeNonToolbarBand(REBAR_BAND_ID_ADDRESS_BAR, m_addressBar->GetHWND(),
		m_config->showAddressBar.get());
	mainRebarBands.push_back(internalBandInfo);

	m_rebarConnections.push_back(m_config->showAddressBar.addObserver(
		std::bind_front(&Explorerplusplus::ShowMainRebarBand, this, m_addressBar->GetHWND())));

	CreateBookmarksToolbar();
	internalBandInfo = InitializeToolbarBand(REBAR_BAND_ID_BOOKMARKS_TOOLBAR,
		m_bookmarksToolbar->GetView()->GetHWND(), m_config->showBookmarksToolbar.get());
	mainRebarBands.push_back(internalBandInfo);

	m_rebarConnections.push_back(m_config->showBookmarksToolbar.addObserver(std::bind_front(
		&Explorerplusplus::ShowMainRebarBand, this, m_bookmarksToolbar->GetView()->GetHWND())));

	CreateDrivesToolbar();
	internalBandInfo = InitializeToolbarBand(REBAR_BAND_ID_DRIVES_TOOLBAR,
		m_drivesToolbar->GetView()->GetHWND(), m_config->showDrivesToolbar.get());
	mainRebarBands.push_back(internalBandInfo);

	m_rebarConnections.push_back(m_config->showDrivesToolbar.addObserver(std::bind_front(
		&Explorerplusplus::ShowMainRebarBand, this, m_drivesToolbar->GetView()->GetHWND())));

	CreateApplicationToolbar();
	internalBandInfo = InitializeToolbarBand(REBAR_BAND_ID_APPLICATIONS_TOOLBAR,
		m_applicationToolbar->GetView()->GetHWND(), m_config->showApplicationToolbar.get());
	mainRebarBands.push_back(internalBandInfo);

	m_rebarConnections.push_back(m_config->showApplicationToolbar.addObserver(std::bind_front(
		&Explorerplusplus::ShowMainRebarBand, this, m_applicationToolbar->GetView()->GetHWND())));

	UpdateMainRebarBandsFromLoadedInfo(mainRebarBands);

	return mainRebarBands;
}

Explorerplusplus::InternalRebarBandInfo Explorerplusplus::InitializeToolbarBand(UINT id,
	HWND toolbar, bool showBand)
{
	auto toolbarSize = static_cast<DWORD>(SendMessage(toolbar, TB_GETBUTTONSIZE, 0, 0));

	SIZE size;
	auto res = SendMessage(toolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&size));
	DCHECK(res);

	InternalRebarBandInfo internalBandInfo = {};
	internalBandInfo.id = id;
	internalBandInfo.child = toolbar;
	internalBandInfo.height = HIWORD(toolbarSize);
	internalBandInfo.newLine = true;
	internalBandInfo.useChevron = true;
	internalBandInfo.showBand = showBand;
	internalBandInfo.idealLength = size.cx;
	return internalBandInfo;
}

Explorerplusplus::InternalRebarBandInfo Explorerplusplus::InitializeNonToolbarBand(UINT id,
	HWND child, bool showBand)
{
	RECT rect;
	auto res = GetWindowRect(child, &rect);
	DCHECK(res);

	InternalRebarBandInfo internalBandInfo = {};
	internalBandInfo.id = id;
	internalBandInfo.child = child;
	internalBandInfo.height = GetRectHeight(&rect);
	internalBandInfo.newLine = true;
	internalBandInfo.useChevron = false;
	internalBandInfo.showBand = showBand;
	return internalBandInfo;
}

void Explorerplusplus::UpdateMainRebarBandsFromLoadedInfo(
	std::vector<InternalRebarBandInfo> &mainRebarBands)
{
	auto getSortedBandIndex = [this, &mainRebarBands](UINT bandId) -> size_t
	{
		auto itr = std::find_if(m_loadedRebarStorageInfo.begin(), m_loadedRebarStorageInfo.end(),
			[bandId](const auto &loadedBandInfo) { return loadedBandInfo.id == bandId; });

		if (itr == m_loadedRebarStorageInfo.end())
		{
			// Any band that doesn't appear in the loaded data will be placed at the end.
			return mainRebarBands.size() - 1;
		}

		return itr - m_loadedRebarStorageInfo.begin();
	};

	std::stable_sort(mainRebarBands.begin(), mainRebarBands.end(),
		[getSortedBandIndex](const auto &band1, const auto &band2)
		{ return getSortedBandIndex(band1.id) < getSortedBandIndex(band2.id); });

	for (auto &band : mainRebarBands)
	{
		UpdateMainRebarBandFromLoadedInfo(band);
	}
}

void Explorerplusplus::UpdateMainRebarBandFromLoadedInfo(InternalRebarBandInfo &internalBandInfo)
{
	auto itr = std::find_if(m_loadedRebarStorageInfo.begin(), m_loadedRebarStorageInfo.end(),
		[&internalBandInfo](const auto &loadedBandInfo)
		{ return loadedBandInfo.id == internalBandInfo.id; });

	if (itr == m_loadedRebarStorageInfo.end())
	{
		return;
	}

	internalBandInfo.newLine = WI_IsFlagSet(itr->style, RBBS_BREAK);
	internalBandInfo.length = itr->length;
}

void Explorerplusplus::InsertMainRebarBand(const InternalRebarBandInfo &internalBandInfo)
{
	REBARBANDINFO bandInfo = {};
	bandInfo.cbSize = sizeof(bandInfo);
	bandInfo.fMask = RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
	bandInfo.fStyle = 0;
	bandInfo.wID = internalBandInfo.id;
	bandInfo.hwndChild = internalBandInfo.child;
	bandInfo.cx = internalBandInfo.length;
	bandInfo.cxMinChild = 0;
	bandInfo.cyMinChild = internalBandInfo.height;
	bandInfo.cyChild = internalBandInfo.height;

	if (m_config->lockToolbars.get())
	{
		WI_SetFlag(bandInfo.fStyle, RBBS_NOGRIPPER);
	}

	if (internalBandInfo.newLine)
	{
		WI_SetFlag(bandInfo.fStyle, RBBS_BREAK);
	}

	if (internalBandInfo.useChevron)
	{
		WI_SetFlag(bandInfo.fStyle, RBBS_USECHEVRON);
	}

	if (!internalBandInfo.showBand)
	{
		WI_SetFlag(bandInfo.fStyle, RBBS_HIDDEN);
	}

	if (internalBandInfo.idealLength)
	{
		WI_SetFlag(bandInfo.fMask, RBBIM_IDEALSIZE);
		bandInfo.cxIdeal = *internalBandInfo.idealLength;
	}

	auto res = SendMessage(m_hMainRebar, RB_INSERTBAND, static_cast<WPARAM>(-1),
		reinterpret_cast<LPARAM>(&bandInfo));
	DCHECK(res);
}

LRESULT Explorerplusplus::RebarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

	case WM_DESTROY:
		m_rebarConnections.clear();
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

	MenuHelper::CheckItem(menu, IDM_TOOLBARS_ADDRESSBAR, m_config->showAddressBar.get());
	MenuHelper::CheckItem(menu, IDM_TOOLBARS_MAINTOOLBAR, m_config->showMainToolbar.get());
	MenuHelper::CheckItem(menu, IDM_TOOLBARS_BOOKMARKSTOOLBAR,
		m_config->showBookmarksToolbar.get());
	MenuHelper::CheckItem(menu, IDM_TOOLBARS_DRIVES, m_config->showDrivesToolbar.get());
	MenuHelper::CheckItem(menu, IDM_TOOLBARS_APPLICATIONTOOLBAR,
		m_config->showApplicationToolbar.get());
	MenuHelper::CheckItem(menu, IDM_TOOLBARS_LOCKTOOLBARS, m_config->lockToolbars.get());

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
		OnToggleLockToolbars();
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
	m_config->showAddressBar = !m_config->showAddressBar.get();
}

void Explorerplusplus::OnToggleMainToolbar()
{
	m_config->showMainToolbar = !m_config->showMainToolbar.get();
}

void Explorerplusplus::OnToggleBookmarksToolbar()
{
	m_config->showBookmarksToolbar = !m_config->showBookmarksToolbar.get();
}

void Explorerplusplus::OnToggleDrivesToolbar()
{
	m_config->showDrivesToolbar = !m_config->showDrivesToolbar.get();
}

void Explorerplusplus::OnToggleApplicationToolbar()
{
	m_config->showApplicationToolbar = !m_config->showApplicationToolbar.get();
}

void Explorerplusplus::ShowMainRebarBand(HWND toolbar, bool show)
{
	auto numBands = static_cast<UINT>(SendMessage(m_hMainRebar, RB_GETBANDCOUNT, 0, 0));

	for (UINT i = 0; i < numBands; i++)
	{
		REBARBANDINFO bandInfo = {};
		bandInfo.cbSize = sizeof(bandInfo);
		bandInfo.fMask = RBBIM_CHILD;
		auto res =
			SendMessage(m_hMainRebar, RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));

		if (!res)
		{
			DCHECK(false);
			continue;
		}

		if (toolbar == bandInfo.hwndChild)
		{
			SendMessage(m_hMainRebar, RB_SHOWBAND, i, show);
			break;
		}
	}
}

void Explorerplusplus::OnToggleLockToolbars()
{
	m_config->lockToolbars = !m_config->lockToolbars.get();
}

void Explorerplusplus::OnLockToolbarsUpdated(bool lock)
{
	auto numBands = static_cast<UINT>(SendMessage(m_hMainRebar, RB_GETBANDCOUNT, 0, 0));

	for (UINT i = 0; i < numBands; i++)
	{
		REBARBANDINFO bandInfo = {};
		bandInfo.cbSize = sizeof(REBARBANDINFO);
		bandInfo.fMask = RBBIM_STYLE;
		auto res =
			SendMessage(m_hMainRebar, RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));

		if (!res)
		{
			DCHECK(false);
			continue;
		}

		// Without this, the control won't correctly update once the gripper has been added or
		// removed. That is, the control won't add or remove the space for the gripper. By toggling
		// the RBBS_GRIPPERALWAYS style, adding or removing the gripper will work as expected. WTL
		// also does this when toggling the gripper - see
		// https://sourceforge.net/p/wtl/git/ci/faa1f28fb7fe9277532ed563101b489655b40131/tree/Include/atlctrls.h#l8604.
		WI_SetFlag(bandInfo.fStyle, RBBS_GRIPPERALWAYS);
		res = SendMessage(m_hMainRebar, RB_SETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));
		DCHECK(res);
		WI_ClearFlag(bandInfo.fStyle, RBBS_GRIPPERALWAYS);

		if (lock)
		{
			WI_SetFlag(bandInfo.fStyle, RBBS_NOGRIPPER);
		}
		else
		{
			WI_ClearFlag(bandInfo.fStyle, RBBS_NOGRIPPER);
		}

		res = SendMessage(m_hMainRebar, RB_SETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));
		DCHECK(res);
	}
}

void Explorerplusplus::OnCustomizeMainToolbar()
{
	m_mainToolbar->StartCustomization();
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
	m_addressBar->sizeUpdatedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnAddressBarSizeUpdated, this));
}

void Explorerplusplus::OnAddressBarSizeUpdated()
{
	RECT rect;
	GetWindowRect(m_addressBar->GetHWND(), &rect);
	UpdateRebarBandSize(m_hMainRebar, m_addressBar->GetHWND(), 0, GetRectHeight(&rect));
}

void Explorerplusplus::CreateMainToolbar()
{
	m_mainToolbar = MainToolbar::Create(m_hMainRebar, m_resourceInstance, this, this,
		&m_shellIconLoader, m_config, m_loadedMainToolbarButtons);
	m_mainToolbar->sizeUpdatedSignal.AddObserver(
		std::bind(&Explorerplusplus::OnRebarToolbarSizeUpdated, this, m_mainToolbar->GetHWND()));
}

void Explorerplusplus::CreateBookmarksToolbar()
{
	auto bookmarksToolbarView = new BookmarksToolbarView(m_hMainRebar, m_config);

	m_bookmarksToolbar = BookmarksToolbar::Create(bookmarksToolbarView, this, this, &m_iconFetcher,
		m_app->GetBookmarkTree());
	m_bookmarksToolbar->GetView()->AddToolbarSizeUpdatedObserver(
		std::bind(&Explorerplusplus::OnRebarToolbarSizeUpdated, this,
			m_bookmarksToolbar->GetView()->GetHWND()));
}

void Explorerplusplus::CreateDrivesToolbar()
{
	auto drivesToolbarView = DrivesToolbarView::Create(m_hMainRebar, m_config);

	auto driveEnumerator = std::make_unique<DriveEnumeratorImpl>();
	auto driveWatcher = std::make_unique<DriveWatcherImpl>(m_hContainer);
	auto driveModel =
		std::make_unique<DriveModel>(std::move(driveEnumerator), std::move(driveWatcher));

	m_drivesToolbar = DrivesToolbar::Create(drivesToolbarView, std::move(driveModel), this, this);
	m_drivesToolbar->GetView()->AddToolbarSizeUpdatedObserver(std::bind(
		&Explorerplusplus::OnRebarToolbarSizeUpdated, this, m_drivesToolbar->GetView()->GetHWND()));
}

void Explorerplusplus::CreateApplicationToolbar()
{
	auto applicationToolbarView =
		Applications::ApplicationToolbarView::Create(m_hMainRebar, m_config);

	m_applicationToolbar = Applications::ApplicationToolbar::Create(applicationToolbarView,
		m_app->GetApplicationModel(), this);
	m_applicationToolbar->GetView()->AddToolbarSizeUpdatedObserver(
		std::bind(&Explorerplusplus::OnRebarToolbarSizeUpdated, this,
			m_applicationToolbar->GetView()->GetHWND()));
}

void Explorerplusplus::OnRebarToolbarSizeUpdated(HWND toolbar)
{
	SIZE size;
	[[maybe_unused]] auto res =
		SendMessage(toolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&size));
	assert(res);

	UpdateRebarBandSize(m_hMainRebar, toolbar, size.cx, size.cy);
}

HMENU Explorerplusplus::CreateRebarHistoryMenu(BOOL bBack)
{
	HMENU hSubMenu = nullptr;
	std::vector<HistoryEntry *> history;
	int iBase;

	const Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();

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

std::vector<RebarBandStorageInfo> Explorerplusplus::GetMainRebarStorageInfo()
{
	std::vector<RebarBandStorageInfo> rebarStorageInfo;
	auto numBands = static_cast<UINT>(SendMessage(m_hMainRebar, RB_GETBANDCOUNT, 0, 0));

	for (UINT i = 0; i < numBands; i++)
	{
		REBARBANDINFO bandInfo = {};
		bandInfo.cbSize = sizeof(bandInfo);
		bandInfo.fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_STYLE;
		auto res =
			SendMessage(m_hMainRebar, RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&bandInfo));

		if (!res)
		{
			DCHECK(false);
			continue;
		}

		RebarBandStorageInfo bandStorageInfo;
		bandStorageInfo.id = bandInfo.wID;
		bandStorageInfo.style = bandInfo.fStyle;
		bandStorageInfo.length = bandInfo.cx;
		rebarStorageInfo.push_back(bandStorageInfo);
	}

	return rebarStorageInfo;
}
