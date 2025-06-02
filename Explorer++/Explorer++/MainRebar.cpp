// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "AddressBarView.h"
#include "App.h"
#include "ApplicationToolbar.h"
#include "ApplicationToolbarView.h"
#include "Bookmarks/UI/BookmarksToolbar.h"
#include "Bookmarks/UI/Views/BookmarksToolbarView.h"
#include "Config.h"
#include "DrivesToolbar.h"
#include "DrivesToolbarView.h"
#include "Explorer++_internal.h"
#include "MainRebarStorage.h"
#include "MainRebarView.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "PopupMenuView.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainerImpl.h"
#include "ToolbarContextMenu.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowHelper.h"

void Explorerplusplus::CreateMainRebarAndChildren(const WindowStorageData *storageData)
{
	m_mainRebarView = MainRebarView::Create(m_hContainer);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_mainRebarView->GetHWND(),
		std::bind_front(&Explorerplusplus::RebarSubclass, this)));

	auto bands = InitializeMainRebarBands(storageData);
	m_mainRebarView->AddBands(bands);

	m_mainRebarView->LockBands(m_config->lockToolbars.get());
	m_rebarConnections.push_back(m_config->lockToolbars.addObserver(
		std::bind_front(&RebarView::LockBands, m_mainRebarView)));
}

std::vector<RebarView::Band> Explorerplusplus::InitializeMainRebarBands(
	const WindowStorageData *storageData)
{
	std::vector<RebarView::Band> mainRebarBands;

	CreateMainToolbar(storageData ? storageData->mainToolbarButtons : std::nullopt);
	auto band = InitializeToolbarBand(REBAR_BAND_ID_MAIN_TOOLBAR, m_mainToolbar->GetHWND(),
		m_config->showMainToolbar.get());
	mainRebarBands.push_back(band);

	m_rebarConnections.push_back(m_config->showMainToolbar.addObserver(
		std::bind_front(&RebarView::ShowBand, m_mainRebarView, m_mainToolbar->GetHWND())));

	CreateAddressBar();
	band = InitializeNonToolbarBand(REBAR_BAND_ID_ADDRESS_BAR, m_addressBar->GetView()->GetHWND(),
		m_config->showAddressBar.get());
	mainRebarBands.push_back(band);

	m_rebarConnections.push_back(m_config->showAddressBar.addObserver(std::bind_front(
		&RebarView::ShowBand, m_mainRebarView, m_addressBar->GetView()->GetHWND())));

	CreateBookmarksToolbar();
	band = InitializeToolbarBand(REBAR_BAND_ID_BOOKMARKS_TOOLBAR,
		m_bookmarksToolbar->GetView()->GetHWND(), m_config->showBookmarksToolbar.get());
	mainRebarBands.push_back(band);

	m_rebarConnections.push_back(m_config->showBookmarksToolbar.addObserver(std::bind_front(
		&RebarView::ShowBand, m_mainRebarView, m_bookmarksToolbar->GetView()->GetHWND())));

	CreateDrivesToolbar();
	band = InitializeToolbarBand(REBAR_BAND_ID_DRIVES_TOOLBAR,
		m_drivesToolbar->GetView()->GetHWND(), m_config->showDrivesToolbar.get());
	mainRebarBands.push_back(band);

	m_rebarConnections.push_back(m_config->showDrivesToolbar.addObserver(std::bind_front(
		&RebarView::ShowBand, m_mainRebarView, m_drivesToolbar->GetView()->GetHWND())));

	CreateApplicationToolbar();
	band = InitializeToolbarBand(REBAR_BAND_ID_APPLICATIONS_TOOLBAR,
		m_applicationToolbar->GetView()->GetHWND(), m_config->showApplicationToolbar.get());
	mainRebarBands.push_back(band);

	m_rebarConnections.push_back(m_config->showApplicationToolbar.addObserver(std::bind_front(
		&RebarView::ShowBand, m_mainRebarView, m_applicationToolbar->GetView()->GetHWND())));

	if (storageData)
	{
		UpdateMainRebarBandsFromLoadedInfo(mainRebarBands, storageData->mainRebarInfo);
	}

	return mainRebarBands;
}

RebarView::Band Explorerplusplus::InitializeToolbarBand(UINT id, HWND toolbar, bool showBand)
{
	auto toolbarSize = static_cast<DWORD>(SendMessage(toolbar, TB_GETBUTTONSIZE, 0, 0));

	SIZE size;
	auto res = SendMessage(toolbar, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&size));
	DCHECK(res);

	RebarView::Band band = {};
	band.id = id;
	band.child = toolbar;
	band.height = HIWORD(toolbarSize);
	band.newLine = true;
	band.useChevron = true;
	band.show = showBand;
	band.idealLength = size.cx;
	return band;
}

RebarView::Band Explorerplusplus::InitializeNonToolbarBand(UINT id, HWND child, bool showBand)
{
	RECT rect;
	auto res = GetWindowRect(child, &rect);
	DCHECK(res);

	RebarView::Band band = {};
	band.id = id;
	band.child = child;
	band.height = GetRectHeight(&rect);
	band.newLine = true;
	band.useChevron = false;
	band.show = showBand;
	return band;
}

void Explorerplusplus::UpdateMainRebarBandsFromLoadedInfo(
	std::vector<RebarView::Band> &mainRebarBands,
	const std::vector<RebarBandStorageInfo> &rebarStorageInfo)
{
	auto getSortedBandIndex = [&mainRebarBands, &rebarStorageInfo](UINT bandId) -> size_t
	{
		auto itr = std::find_if(rebarStorageInfo.begin(), rebarStorageInfo.end(),
			[bandId](const auto &loadedBandInfo) { return loadedBandInfo.id == bandId; });

		if (itr == rebarStorageInfo.end())
		{
			// Any band that doesn't appear in the loaded data will be placed at the end.
			return mainRebarBands.size() - 1;
		}

		return itr - rebarStorageInfo.begin();
	};

	std::stable_sort(mainRebarBands.begin(), mainRebarBands.end(),
		[getSortedBandIndex](const auto &band1, const auto &band2)
		{ return getSortedBandIndex(band1.id) < getSortedBandIndex(band2.id); });

	for (auto &band : mainRebarBands)
	{
		UpdateMainRebarBandFromLoadedInfo(band, rebarStorageInfo);
	}
}

void Explorerplusplus::UpdateMainRebarBandFromLoadedInfo(RebarView::Band &band,
	const std::vector<RebarBandStorageInfo> &rebarStorageInfo)
{
	auto itr = std::find_if(rebarStorageInfo.begin(), rebarStorageInfo.end(),
		[&band](const auto &loadedBandInfo) { return loadedBandInfo.id == band.id; });

	if (itr == rebarStorageInfo.end())
	{
		return;
	}

	band.newLine = WI_IsFlagSet(itr->style, RBBS_BREAK);
	band.length = itr->length;
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
			if (OnToolbarRightClick(reinterpret_cast<NMMOUSE *>(lParam)))
			{
				return TRUE;
			}
			break;
		}
		break;

	case WM_DESTROY:
		m_rebarConnections.clear();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

bool Explorerplusplus::OnToolbarRightClick(const NMMOUSE *mouseInfo)
{
	if (mouseInfo->dwItemSpec != -1)
	{
		return false;
	}

	ToolbarContextMenu::Source source = ToolbarContextMenu::Source::MainToolbar;

	if (mouseInfo->hdr.hwndFrom == m_addressBar->GetView()->GetHWND())
	{
		source = ToolbarContextMenu::Source::AddressBar;
	}
	else if (mouseInfo->hdr.hwndFrom == m_mainToolbar->GetHWND())
	{
		source = ToolbarContextMenu::Source::MainToolbar;
	}
	else if (mouseInfo->hdr.hwndFrom == m_bookmarksToolbar->GetView()->GetHWND())
	{
		source = ToolbarContextMenu::Source::BookmarksToolbar;
	}
	else if (mouseInfo->hdr.hwndFrom == m_drivesToolbar->GetView()->GetHWND())
	{
		source = ToolbarContextMenu::Source::DrivesToolbar;
	}
	else if (mouseInfo->hdr.hwndFrom == m_applicationToolbar->GetView()->GetHWND())
	{
		source = ToolbarContextMenu::Source::ApplicationToolbar;
	}
	else
	{
		DCHECK(false);
	}

	POINT ptScreen = mouseInfo->pt;
	ClientToScreen(mouseInfo->hdr.hwndFrom, &ptScreen);

	PopupMenuView popupMenu;
	ToolbarContextMenu toolbarContextMenu(&popupMenu, source, m_app, this);
	popupMenu.Show(m_hContainer, ptScreen);

	return true;
}

void Explorerplusplus::CreateAddressBar()
{
	auto *addressBarView = AddressBarView::Create(m_mainRebarView->GetHWND(), m_config);
	addressBarView->sizeUpdatedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnAddressBarSizeUpdated, this));

	m_addressBar = AddressBar::Create(addressBarView, this, m_app->GetTabEvents(),
		m_app->GetShellBrowserEvents(), m_app->GetNavigationEvents(), m_app->GetRuntime(),
		m_app->GetIconFetcher());
}

void Explorerplusplus::OnAddressBarSizeUpdated()
{
	RECT rect;
	GetWindowRect(m_addressBar->GetView()->GetHWND(), &rect);
	m_mainRebarView->UpdateBandSize(m_addressBar->GetView()->GetHWND(), 0, GetRectHeight(&rect));
}

void Explorerplusplus::CreateMainToolbar(
	const std::optional<MainToolbarStorage::MainToolbarButtons> &initialButtons)
{
	m_mainToolbar = MainToolbar::Create(m_mainRebarView->GetHWND(), m_app, this, this,
		m_app->GetResourceLoader(), &m_shellIconLoader, initialButtons);
	m_mainToolbar->sizeUpdatedSignal.AddObserver(
		std::bind(&Explorerplusplus::OnRebarToolbarSizeUpdated, this, m_mainToolbar->GetHWND()));
}

void Explorerplusplus::CreateBookmarksToolbar()
{
	auto bookmarksToolbarView = BookmarksToolbarView::Create(m_mainRebarView->GetHWND(), m_config);

	m_bookmarksToolbar = BookmarksToolbar::Create(bookmarksToolbarView, this,
		m_app->GetAcceleratorManager(), m_app->GetResourceLoader(), &m_iconFetcher,
		m_app->GetBookmarkTree(), m_app->GetClipboardStore());
	m_bookmarksToolbar->GetView()->AddToolbarSizeUpdatedObserver(
		std::bind(&Explorerplusplus::OnRebarToolbarSizeUpdated, this,
			m_bookmarksToolbar->GetView()->GetHWND()));
}

void Explorerplusplus::CreateDrivesToolbar()
{
	auto drivesToolbarView = DrivesToolbarView::Create(m_mainRebarView->GetHWND(), m_config);
	m_drivesToolbar = DrivesToolbar::Create(drivesToolbarView, m_app->GetDriveModel(), this,
		m_app->GetResourceLoader());
	m_drivesToolbar->GetView()->AddToolbarSizeUpdatedObserver(std::bind(
		&Explorerplusplus::OnRebarToolbarSizeUpdated, this, m_drivesToolbar->GetView()->GetHWND()));
}

void Explorerplusplus::CreateApplicationToolbar()
{
	auto applicationToolbarView =
		Applications::ApplicationToolbarView::Create(m_mainRebarView->GetHWND(), m_config);

	m_applicationToolbar = Applications::ApplicationToolbar::Create(applicationToolbarView,
		m_app->GetApplicationModel(), &m_applicationExecutor, this, m_app->GetAcceleratorManager(),
		m_app->GetResourceLoader());
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

	m_mainRebarView->UpdateBandSize(toolbar, size.cx, size.cy);
}

HMENU Explorerplusplus::CreateRebarHistoryMenu(BOOL bBack)
{
	HMENU hSubMenu = nullptr;
	std::vector<HistoryEntry *> history;
	int iBase;

	const Tab &tab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();

	if (bBack)
	{
		iBase = ID_REBAR_MENU_BACK_START;
		history = tab.GetShellBrowserImpl()->GetNavigationController()->GetBackHistory();
	}
	else
	{
		iBase = ID_REBAR_MENU_FORWARD_START;
		history = tab.GetShellBrowserImpl()->GetNavigationController()->GetForwardHistory();
	}

	if (!history.empty())
	{
		int i = 0;

		hSubMenu = CreateMenu();

		for (auto *entry : history)
		{
			std::wstring displayName =
				GetDisplayNameWithFallback(entry->GetPidl().Raw(), SHGDN_INFOLDER);

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
