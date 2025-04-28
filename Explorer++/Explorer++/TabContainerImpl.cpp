// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContainerImpl.h"
#include "App.h"
#include "Bookmarks/BookmarkHelper.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "CoreInterface.h"
#include "Icon.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "PopupMenuView.h"
#include "PreservedTab.h"
#include "RenameTabDialog.h"
#include "ResourceHelper.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/PreservedHistoryEntry.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "SystemFontHelper.h"
#include "TabBacking.h"
#include "TabContainerBackgroundContextMenu.h"
#include "TabStorage.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/TabHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/iDirectoryMonitor.h"
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>
#include <glog/logging.h>

using namespace std::chrono_literals;

const UINT TAB_CONTROL_STYLES = WS_VISIBLE | WS_CHILD | TCS_FOCUSNEVER | TCS_SINGLELINE
	| TCS_TOOLTIPS | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

// clang-format off
const std::map<UINT, Icon> TAB_RIGHT_CLICK_MENU_IMAGE_MAPPINGS = {
	{ IDM_FILE_NEWTAB, Icon::NewTab },
	{ IDM_TAB_REFRESH, Icon::Refresh },
	{ IDM_TAB_CLOSETAB, Icon::CloseTab }
};
// clang-format on

TabContainerImpl *TabContainerImpl::Create(HWND parent, BrowserWindow *browser,
	TabNavigationInterface *tabNavigation, App *app, CoreInterface *coreInterface,
	FileActionHandler *fileActionHandler, CachedIcons *cachedIcons, BookmarkTree *bookmarkTree,
	HINSTANCE resourceInstance, const Config *config)
{
	return new TabContainerImpl(parent, browser, tabNavigation, app, coreInterface,
		fileActionHandler, cachedIcons, bookmarkTree, resourceInstance, config);
}

TabContainerImpl::TabContainerImpl(HWND parent, BrowserWindow *browser,
	TabNavigationInterface *tabNavigation, App *app, CoreInterface *coreInterface,
	FileActionHandler *fileActionHandler, CachedIcons *cachedIcons, BookmarkTree *bookmarkTree,
	HINSTANCE resourceInstance, const Config *config) :
	ShellDropTargetWindow(CreateTabControl(parent)),
	m_browser(browser),
	m_tabNavigation(tabNavigation),
	m_app(app),
	m_coreInterface(coreInterface),
	m_fileActionHandler(fileActionHandler),
	m_fontSetter(m_hwnd, config, GetDefaultSystemFontForDefaultDpi()),
	m_tooltipFontSetter(TabCtrl_GetToolTips(m_hwnd), config),
	m_timerManager(m_hwnd),
	m_iconFetcher(m_hwnd, cachedIcons),
	m_cachedIcons(cachedIcons),
	m_resourceInstance(resourceInstance),
	m_config(config),
	m_iPreviousTabSelectionId(-1),
	m_bTabBeenDragged(FALSE),
	m_bookmarkTree(bookmarkTree)
{
	Initialize(parent);
}

HWND TabContainerImpl::CreateTabControl(HWND parent)
{
	return ::CreateTabControl(parent, TAB_CONTROL_STYLES);
}

void TabContainerImpl::Initialize(HWND parent)
{
	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(m_defaultFolderIconSystemImageListIndex));

	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(m_hwnd);

	SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList));

	int dpiScaledSize = MulDiv(ICON_SIZE_96DPI, dpi, USER_DEFAULT_SCREEN_DPI);
	m_tabCtrlImageList.reset(
		ImageList_Create(dpiScaledSize, dpiScaledSize, ILC_COLOR32 | ILC_MASK, 0, 100));
	TabCtrl_SetImageList(m_hwnd, m_tabCtrlImageList.get());

	AddDefaultTabIcons(m_tabCtrlImageList.get());

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hwnd,
		std::bind_front(&TabContainerImpl::WndProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(parent,
		std::bind_front(&TabContainerImpl::ParentWndProc, this)));

	m_connections.push_back(m_app->GetTabEvents()->AddUpdatedObserver(
		std::bind_front(&TabContainerImpl::OnTabUpdated, this),
		TabEventScope::ForBrowser(*m_browser)));

	m_connections.push_back(m_app->GetShellBrowserEvents()->AddDirectoryPropertiesChangedObserver(
		std::bind_front(&TabContainerImpl::OnDirectoryPropertiesChanged, this),
		ShellBrowserEventScope::ForBrowser(*m_browser)));

	m_connections.push_back(m_app->GetNavigationEvents()->AddCommittedObserver(
		std::bind_front(&TabContainerImpl::OnNavigationCommitted, this),
		NavigationEventScope::ForBrowser(*m_browser)));

	m_connections.push_back(m_config->alwaysShowTabBar.addObserver(
		std::bind_front(&TabContainerImpl::OnAlwaysShowTabBarUpdated, this)));

	m_fontSetter.fontUpdatedSignal.AddObserver(
		std::bind_front(&TabContainerImpl::OnFontUpdated, this));
}

void TabContainerImpl::AddDefaultTabIcons(HIMAGELIST himlTab)
{
	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hwnd);
	wil::unique_hbitmap bitmap = m_app->GetIconResourceLoader()->LoadBitmapFromPNGForDpi(Icon::Lock,
		ICON_SIZE_96DPI, ICON_SIZE_96DPI, dpi);
	m_tabIconLockIndex = ImageList_Add(himlTab, bitmap.get(), nullptr);

	m_defaultFolderIconIndex = ImageHelper::CopyImageListIcon(m_tabCtrlImageList.get(),
		reinterpret_cast<HIMAGELIST>(m_systemImageList.get()),
		m_defaultFolderIconSystemImageListIndex);
}

bool TabContainerImpl::IsDefaultIcon(int iconIndex)
{
	if (iconIndex == m_tabIconLockIndex || iconIndex == m_defaultFolderIconIndex)
	{
		return true;
	}

	return false;
}

LRESULT TabContainerImpl::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);

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

	case WM_LBUTTONDBLCLK:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		OnLButtonDoubleClick(pt);
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

	case WM_MENUSELECT:
		/* Forward the message to the main window so it can
		handle menu help. */
		SendMessage(m_coreInterface->GetMainWindow(), WM_MENUSELECT, wParam, lParam);
		break;

	case WM_CAPTURECHANGED:
	{
		if ((HWND) lParam != hwnd)
		{
			ReleaseCapture();
		}

		m_bTabBeenDragged = FALSE;
	}
	break;

	case WM_NCDESTROY:
		delete this;
		return 0;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void TabContainerImpl::OnTabCtrlLButtonDown(POINT *pt)
{
	TCHITTESTINFO info;
	info.pt = *pt;
	int itemNum = TabCtrl_HitTest(m_hwnd, &info);

	if (info.flags != TCHT_NOWHERE)
	{
		/* Save the bounds of the dragged tab. */
		TabCtrl_GetItemRect(m_hwnd, itemNum, &m_rcDraggedTab);

		/* Capture mouse movement exclusively until
		the mouse button is released. */
		SetCapture(m_hwnd);

		m_bTabBeenDragged = TRUE;
		m_draggedTabStartIndex = itemNum;
		m_draggedTabEndIndex = itemNum;
	}
}

void TabContainerImpl::OnTabCtrlLButtonUp()
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
		m_app->GetTabEvents()->NotifyMoved(tab, m_draggedTabStartIndex, m_draggedTabEndIndex);
	}
}

void TabContainerImpl::OnTabCtrlMouseMove(POINT *pt)
{
	if (!m_bTabBeenDragged)
	{
		return;
	}

	/* Dragged tab. */
	int iSelected = TabCtrl_GetCurFocus(m_hwnd);

	TCHITTESTINFO hitTestInfo;
	hitTestInfo.pt = *pt;
	int iSwap = TabCtrl_HitTest(m_hwnd, &hitTestInfo);

	/* Check:
	- If the cursor is over an item.
	- If the cursor is not over the dragged item itself.
	- If the cursor has passed to the left of the dragged tab, or
	- If the cursor has passed to the right of the dragged tab. */
	if (hitTestInfo.flags != TCHT_NOWHERE && iSwap != iSelected
		&& (pt->x < m_rcDraggedTab.left || pt->x > m_rcDraggedTab.right))
	{
		RECT rcSwap;

		TabCtrl_GetItemRect(m_hwnd, iSwap, &rcSwap);

		/* These values need to be adjusted, since
		tabs are adjusted whenever the dragged tab
		passes a boundary, not when the cursor is
		released. */
		if (pt->x > m_rcDraggedTab.right)
		{
			/* Cursor has gone past the right edge of
			the dragged tab. */
			m_rcDraggedTab.left = m_rcDraggedTab.right;
			m_rcDraggedTab.right = rcSwap.right;
		}
		else
		{
			/* Cursor has gone past the left edge of
			the dragged tab. */
			m_rcDraggedTab.right = m_rcDraggedTab.left;
			m_rcDraggedTab.left = rcSwap.left;
		}

		/* Swap the dragged tab with the tab the cursor
		finished up on. */
		TabCtrl_SwapItems(m_hwnd, iSelected, iSwap);

		/* The index of the selected tab has now changed
		(but the actual tab/browser selected remains the
		same). */
		TabCtrl_SetCurFocus(m_hwnd, iSwap);

		m_draggedTabEndIndex = iSwap;
	}
}

void TabContainerImpl::OnMouseWheel(HWND hwnd, int xPos, int yPos, int delta, UINT keys)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(xPos);
	UNREFERENCED_PARAMETER(yPos);
	UNREFERENCED_PARAMETER(keys);

	auto scrollDirection = delta > 0 ? ScrollDirection::Left : ScrollDirection::Right;

	for (int i = 0; i < abs(delta / WHEEL_DELTA); i++)
	{
		ScrollTabControl(scrollDirection);
	}
}

void TabContainerImpl::OnLButtonDoubleClick(const POINT &pt)
{
	TCHITTESTINFO info;
	info.pt = pt;
	const int index = TabCtrl_HitTest(m_hwnd, &info);

	if (info.flags != TCHT_NOWHERE && m_config->doubleClickTabClose)
	{
		const Tab &tab = GetTabByIndex(index);
		CloseTab(tab);
	}
}

void TabContainerImpl::OnTabCtrlMButtonUp(POINT *pt)
{
	TCHITTESTINFO htInfo;
	htInfo.pt = *pt;

	/* Find the tab that the click occurred over. */
	int iTabHit = TabCtrl_HitTest(m_hwnd, &htInfo);

	if (iTabHit != -1)
	{
		const Tab &tab = GetTabByIndex(iTabHit);
		CloseTab(tab);
	}
}

void TabContainerImpl::OnTabCtrlRButtonUp(POINT *pt)
{
	TCHITTESTINFO tcHitTest;
	tcHitTest.pt = *pt;
	const int tabHitIndex = TabCtrl_HitTest(m_hwnd, &tcHitTest);

	if (tcHitTest.flags == TCHT_NOWHERE)
	{
		return;
	}

	POINT ptCopy = *pt;
	BOOL res = ClientToScreen(m_hwnd, &ptCopy);

	if (!res)
	{
		return;
	}

	Tab &tab = GetTabByIndex(tabHitIndex);

	CreateTabContextMenu(tab, ptCopy);
}

void TabContainerImpl::CreateTabContextMenu(Tab &tab, const POINT &pt)
{
	auto parentMenu =
		wil::unique_hmenu(LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_TAB_RCLICK)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	std::vector<wil::unique_hbitmap> menuImages;
	AddImagesToTabContextMenu(menu, menuImages);

	MenuHelper::EnableItem(menu, IDM_TAB_OPENPARENTINNEWTAB,
		!IsNamespaceRoot(tab.GetShellBrowserImpl()->GetDirectoryIdl().get()));
	MenuHelper::CheckItem(menu, IDM_TAB_LOCKTAB, tab.GetLockState() == Tab::LockState::Locked);
	MenuHelper::CheckItem(menu, IDM_TAB_LOCKTABANDADDRESS,
		tab.GetLockState() == Tab::LockState::AddressLocked);
	MenuHelper::EnableItem(menu, IDM_TAB_CLOSETAB, tab.GetLockState() == Tab::LockState::NotLocked);

	UINT command =
		TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERTICAL | TPM_RETURNCMD, pt.x,
			pt.y, 0, m_hwnd, nullptr);

	ProcessTabCommand(command, tab);
}

void TabContainerImpl::AddImagesToTabContextMenu(HMENU menu,
	std::vector<wil::unique_hbitmap> &menuImages)
{
	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hwnd);

	for (const auto &mapping : TAB_RIGHT_CLICK_MENU_IMAGE_MAPPINGS)
	{
		ResourceHelper::SetMenuItemImage(menu, mapping.first, m_app->GetIconResourceLoader(),
			mapping.second, dpi, menuImages);
	}
}

void TabContainerImpl::ProcessTabCommand(UINT uMenuID, Tab &tab)
{
	switch (uMenuID)
	{
	case IDM_FILE_NEWTAB:
		/* Send the resulting command back to the main window for processing. */
		SendMessage(m_coreInterface->GetMainWindow(), WM_COMMAND, MAKEWPARAM(uMenuID, 0), 0);
		break;

	case IDM_TAB_DUPLICATETAB:
		DuplicateTab(tab);
		break;

	case IDM_TAB_MOVETABNEWWINDOW:
		MoveTabToNewWindow(tab);
		break;

	case IDM_TAB_OPENPARENTINNEWTAB:
		OnOpenParentInNewTab(tab);
		break;

	case IDM_TAB_REFRESH:
		OnRefreshTab(tab);
		break;

	case IDM_TAB_REFRESHALL:
		OnRefreshAllTabs();
		break;

	case IDM_TAB_RENAMETAB:
		OnRenameTab(tab);
		break;

	case IDM_TAB_LOCKTAB:
		OnLockTab(tab);
		break;

	case IDM_TAB_LOCKTABANDADDRESS:
		OnLockTabAndAddress(tab);
		break;

	case IDM_TAB_CLOSETAB:
		CloseTab(tab);
		break;

	case IDM_TAB_CLOSEOTHERTABS:
		OnCloseOtherTabs(GetTabIndex(tab));
		break;

	case IDM_TAB_CLOSETABSTORIGHT:
		OnCloseTabsToRight(GetTabIndex(tab));
		break;
	}
}

void TabContainerImpl::OnOpenParentInNewTab(const Tab &tab)
{
	auto pidlCurrent = tab.GetShellBrowserImpl()->GetDirectoryIdl();

	unique_pidl_absolute pidlParent;
	HRESULT hr = GetVirtualParentPath(pidlCurrent.get(), wil::out_param(pidlParent));

	if (SUCCEEDED(hr))
	{
		auto navigateParams = NavigateParams::Normal(pidlParent.get());
		CreateNewTab(navigateParams, TabSettings(_selected = true));
	}
}

void TabContainerImpl::OnRefreshTab(Tab &tab)
{
	tab.GetShellBrowserImpl()->GetNavigationController()->Refresh();
}

void TabContainerImpl::OnRefreshAllTabs()
{
	for (auto &tab : GetAllTabs() | boost::adaptors::map_values)
	{
		tab->GetShellBrowserImpl()->GetNavigationController()->Refresh();
	}
}

void TabContainerImpl::OnRenameTab(Tab &tab)
{
	RenameTabDialog renameTabDialog(m_coreInterface->GetMainWindow(), m_app, &tab);
	renameTabDialog.ShowModalDialog();
}

void TabContainerImpl::OnLockTab(Tab &tab)
{
	if (tab.GetLockState() == Tab::LockState::Locked)
	{
		tab.SetLockState(Tab::LockState::NotLocked);
	}
	else
	{
		tab.SetLockState(Tab::LockState::Locked);
	}
}

void TabContainerImpl::OnLockTabAndAddress(Tab &tab)
{
	if (tab.GetLockState() == Tab::LockState::AddressLocked)
	{
		tab.SetLockState(Tab::LockState::NotLocked);
	}
	else
	{
		tab.SetLockState(Tab::LockState::AddressLocked);
	}
}

void TabContainerImpl::OnCloseOtherTabs(int index)
{
	const int nTabs = GetNumTabs();

	/* Close all tabs except the
	specified one. */
	for (int i = nTabs - 1; i >= 0; i--)
	{
		if (i != index)
		{
			const Tab &tab = GetTabByIndex(i);
			CloseTab(tab);
		}
	}
}

void TabContainerImpl::OnCloseTabsToRight(int index)
{
	int nTabs = GetNumTabs();

	for (int i = nTabs - 1; i > index; i--)
	{
		const Tab &currentTab = GetTabByIndex(i);
		CloseTab(currentTab);
	}
}

LRESULT TabContainerImpl::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);

	case WM_LBUTTONDBLCLK:
		CreateNewTabInDefaultDirectory(TabSettings(_selected = true));
		break;

	case WM_RBUTTONUP:
	{
		POINT ptClient;
		POINTSTOPOINT(ptClient, MAKEPOINTS(lParam));
		ShowBackgroundContextMenu(ptClient);
	}
	break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case TABTOOLBAR_CLOSE:
			CloseTab(GetSelectedTab());
			break;
		}
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code)
		{
		case TTN_GETDISPINFO:
			OnGetDispInfo(reinterpret_cast<NMTTDISPINFO *>(lParam));
			break;

		case TCN_SELCHANGE:
			OnTabSelected(GetSelectedTab());
			break;
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void TabContainerImpl::ShowBackgroundContextMenu(const POINT &ptClient)
{
	POINT ptScreen = ptClient;
	ClientToScreen(m_hwnd, &ptScreen);

	PopupMenuView popupMenu;
	TabContainerBackgroundContextMenu menu(&popupMenu, m_app->GetAcceleratorManager(), this,
		m_app->GetTabRestorer(), m_bookmarkTree, m_coreInterface, m_app->GetResourceLoader(),
		m_app->GetIconResourceLoader(), m_app->GetThemeManager());
	popupMenu.Show(m_hwnd, ptScreen);
}

void TabContainerImpl::OnGetDispInfo(NMTTDISPINFO *dispInfo)
{
	HWND toolTipControl = TabCtrl_GetToolTips(m_hwnd);

	if (dispInfo->hdr.hwndFrom != toolTipControl)
	{
		return;
	}

	static TCHAR tabToolTip[512];

	const Tab &tab = GetTabByIndex(static_cast<int>(dispInfo->hdr.idFrom));

	auto pidlDirectory = tab.GetShellBrowserImpl()->GetDirectoryIdl();
	auto path = GetFolderPathForDisplay(pidlDirectory.get());

	if (!path)
	{
		return;
	}

	StringCchCopy(tabToolTip, std::size(tabToolTip), path->c_str());

	dispInfo->lpszText = tabToolTip;
}

void TabContainerImpl::OnTabCreated(const Tab &tab, bool selected)
{
	if (!m_config->alwaysShowTabBar.get() && (GetNumTabs() > 1))
	{
		m_coreInterface->ShowTabBar();
	}

	m_app->GetTabEvents()->NotifyCreated(tab, selected);
}

void TabContainerImpl::OnTabRemoved(const Tab &tab)
{
	if (!m_config->alwaysShowTabBar.get() && (GetNumTabs() == 1))
	{
		m_coreInterface->HideTabBar();
	}

	m_app->GetTabEvents()->NotifyRemoved(tab);
}

void TabContainerImpl::OnTabSelected(const Tab &tab)
{
	if (m_iPreviousTabSelectionId != -1)
	{
		m_tabSelectionHistory.push_back(m_iPreviousTabSelectionId);
	}

	m_iPreviousTabSelectionId = tab.GetId();

	m_app->GetTabEvents()->NotifySelected(tab);
}

void TabContainerImpl::OnAlwaysShowTabBarUpdated(BOOL newValue)
{
	if (newValue)
	{
		m_coreInterface->ShowTabBar();
	}
	else
	{
		if (GetNumTabs() > 1)
		{
			m_coreInterface->ShowTabBar();
		}
		else
		{
			m_coreInterface->HideTabBar();
		}
	}
}

void TabContainerImpl::OnNavigationCommitted(const NavigationRequest *request)
{
	const auto *tab = request->GetShellBrowser()->GetTab();

	UpdateTabNameInWindow(*tab);
	SetTabIcon(*tab);
}

void TabContainerImpl::OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser)
{
	const auto *tab = shellBrowser->GetTab();

	UpdateTabNameInWindow(*tab);
	SetTabIcon(*tab);
}

void TabContainerImpl::OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
{
	switch (propertyType)
	{
	case Tab::PropertyType::LockState:
		SetTabIcon(tab);
		break;

	case Tab::PropertyType::Name:
		UpdateTabNameInWindow(tab);
		break;
	}
}

void TabContainerImpl::UpdateTabNameInWindow(const Tab &tab)
{
	std::wstring name = tab.GetName();
	boost::replace_all(name, L"&", L"&&");

	int index = GetTabIndex(tab);
	TabCtrl_SetItemText(m_hwnd, index, name.c_str());
}

void TabContainerImpl::SetTabIcon(const Tab &tab)
{
	/* If the tab is locked, use a lock icon. */
	if (tab.GetLockState() == Tab::LockState::Locked
		|| tab.GetLockState() == Tab::LockState::AddressLocked)
	{
		SetTabIconFromImageList(tab, m_tabIconLockIndex);
	}
	else
	{
		auto cachedIconIndex =
			m_cachedIcons->MaybeGetIconIndex(tab.GetShellBrowserImpl()->GetDirectory());

		if (cachedIconIndex)
		{
			SetTabIconFromSystemImageList(tab, *cachedIconIndex);
		}
		else
		{
			SetTabIconFromImageList(tab, m_defaultFolderIconIndex);
		}

		auto pidlDirectory = tab.GetShellBrowserImpl()->GetDirectoryIdl();

		m_iconFetcher.QueueIconTask(pidlDirectory.get(),
			[this, tabId = tab.GetId(), folderId = tab.GetShellBrowserImpl()->GetUniqueFolderId()](
				int iconIndex, int overlayIndex)
			{
				UNREFERENCED_PARAMETER(overlayIndex);

				auto tab = GetTabOptional(tabId);

				if (!tab)
				{
					return;
				}

				if (tab->GetShellBrowserImpl()->GetUniqueFolderId() != folderId)
				{
					return;
				}

				SetTabIconFromSystemImageList(*tab, iconIndex);
			});
	}
}

void TabContainerImpl::SetTabIconFromSystemImageList(const Tab &tab, int systemIconIndex)
{
	if (systemIconIndex == m_defaultFolderIconSystemImageListIndex)
	{
		SetTabIconFromImageList(tab, m_defaultFolderIconIndex);
		return;
	}

	int index = ImageHelper::CopyImageListIcon(m_tabCtrlImageList.get(),
		reinterpret_cast<HIMAGELIST>(m_systemImageList.get()), systemIconIndex);
	SetTabIconFromImageList(tab, index);
}

void TabContainerImpl::SetTabIconFromImageList(const Tab &tab, int imageIndex)
{
	int tabIndex = GetTabIndex(tab);

	TCITEM tcItem;
	tcItem.mask = TCIF_IMAGE;
	BOOL res = TabCtrl_GetItem(m_hwnd, tabIndex, &tcItem);

	if (!res)
	{
		return;
	}

	int previousImageIndex = tcItem.iImage;

	tcItem.mask = TCIF_IMAGE;
	tcItem.iImage = imageIndex;
	res = TabCtrl_SetItem(m_hwnd, tabIndex, &tcItem);

	if (!res)
	{
		return;
	}

	if (!IsDefaultIcon(previousImageIndex))
	{
		TabCtrl_RemoveImage(m_hwnd, previousImageIndex);
	}
}

void TabContainerImpl::CreateNewTabInDefaultDirectory(const TabSettings &tabSettings)
{
	CreateNewTab(m_config->defaultTabDirectory, tabSettings);
}

// Note that although it's guaranteed that the tab will be created, it's not guaranteed that it will
// be navigated to the directory that's provided. For example, the directory may not exist, or the
// user may not have access to it, or the path may be temporarily unavailable (e.g. because a
// network location has been disconnected). Therefore, it's not safe to make any assumptions about
// the actual directory the tab is in once it's been created.
Tab &TabContainerImpl::CreateNewTab(const std::wstring &directory, const TabSettings &tabSettings,
	const FolderSettings *folderSettings, const FolderColumns *initialColumns)
{
	unique_pidl_absolute pidl;
	HRESULT hr = ParseDisplayNameForNavigation(directory.c_str(), pidl);

	if (FAILED(hr))
	{
		hr = ParseDisplayNameForNavigation(m_config->defaultTabDirectory.c_str(), pidl);

		if (FAILED(hr))
		{
			hr = ParseDisplayNameForNavigation(m_config->defaultTabDirectoryStatic.c_str(), pidl);
		}
	}

	auto navigateParams = NavigateParams::Normal(pidl.get());
	return CreateNewTab(navigateParams, tabSettings, folderSettings, initialColumns);
}

Tab &TabContainerImpl::CreateNewTab(const PreservedTab &preservedTab)
{
	auto shellBrowser = std::make_unique<ShellBrowserImpl>(m_coreInterface->GetMainWindow(), m_app,
		m_coreInterface, m_tabNavigation, m_fileActionHandler, preservedTab.history,
		preservedTab.currentEntry, preservedTab.preservedFolderState);
	Tab::InitialData initialTabData{ .useCustomName = preservedTab.useCustomName,
		.customName = preservedTab.customName,
		.lockState = preservedTab.lockState };
	auto tab = std::make_unique<Tab>(std::move(shellBrowser), m_browser, this,
		m_app->GetTabEvents(), initialTabData);
	auto *rawTab = tab.get();
	m_tabs.insert({ tab->GetId(), std::move(tab) });

	int finalIndex;

	if (preservedTab.browserId == m_browser->GetId())
	{
		finalIndex = preservedTab.index;
	}
	else
	{
		// This tab is being restored from a different browser, so its index isn't relevant and it
		// should simply be added to the end of the current set of tabs.
		finalIndex = static_cast<int>(m_tabs.size()) - 1;
	}

	TabSettings tabSettings(_index = finalIndex, _selected = true);

	PreservedHistoryEntry *entry = preservedTab.history.at(preservedTab.currentEntry).get();
	auto navigateParams =
		NavigateParams::Normal(entry->GetPidl().Raw(), HistoryEntryType::ReplaceCurrentEntry);
	return SetUpNewTab(*rawTab, navigateParams, tabSettings);
}

Tab &TabContainerImpl::CreateNewTab(NavigateParams &navigateParams, const TabSettings &tabSettings,
	const FolderSettings *folderSettings, const FolderColumns *initialColumns)
{
	FolderSettings folderSettingsFinal;

	if (folderSettings)
	{
		folderSettingsFinal = *folderSettings;
	}
	else
	{
		folderSettingsFinal = m_app->GetConfig()->defaultFolderSettings;
	}

	auto shellBrowser = std::make_unique<ShellBrowserImpl>(m_coreInterface->GetMainWindow(), m_app,
		m_coreInterface, m_tabNavigation, m_fileActionHandler, navigateParams.pidl,
		folderSettingsFinal, initialColumns);

	Tab::InitialData initialTabData;

	if (tabSettings.lockState)
	{
		initialTabData.lockState = *tabSettings.lockState;
	}

	if (tabSettings.name)
	{
		initialTabData.useCustomName = true;
		initialTabData.customName = *tabSettings.name;
	}

	auto tab = std::make_unique<Tab>(std::move(shellBrowser), m_browser, this,
		m_app->GetTabEvents(), initialTabData);
	auto *rawTab = tab.get();
	m_tabs.insert({ tab->GetId(), std::move(tab) });

	return SetUpNewTab(*rawTab, navigateParams, tabSettings);
}

Tab &TabContainerImpl::SetUpNewTab(Tab &tab, NavigateParams &navigateParams,
	const TabSettings &tabSettings)
{
	int index;

	if (tabSettings.index)
	{
		index = *tabSettings.index;
	}
	else
	{
		// When the application is first started, the number of tabs will be 0 initially, so there
		// won't be any selected tab. In that case, the openNewTabNextToCurrent setting should
		// effectively be ignored.
		if (m_config->openNewTabNextToCurrent && GetSelectedTabIndexOptional())
		{
			index = GetSelectedTabIndex() + 1;
		}
		else
		{
			index = TabCtrl_GetItemCount(m_hwnd);
		}
	}

	/* Browse folder sends a message back to the main window, which
	attempts to contact the new tab (needs to be created before browsing
	the folder). */
	index = InsertNewTab(tab, navigateParams, tabSettings, index);

	bool selected = false;

	if (tabSettings.selected)
	{
		selected = *tabSettings.selected;
	}

	if (m_tabs.size() == 1)
	{
		// This is the first tab being inserted, so it should be selected (to ensure there's always
		// a selected tab), regardless of what the caller passes in.
		selected = true;
	}

	if (selected)
	{
		TabCtrl_SetCurSel(m_hwnd, index);
	}

	OnTabCreated(tab, selected);

	if (selected)
	{
		OnTabSelected(tab);
	}

	tab.GetShellBrowserImpl()->GetNavigationController()->Navigate(navigateParams);

	return tab;
}

int TabContainerImpl::InsertNewTab(const Tab &tab, const NavigateParams &navigateParams,
	const TabSettings &tabSettings, int index)
{
	std::wstring name;

	if (tabSettings.name && !tabSettings.name->empty())
	{
		name = *tabSettings.name;
	}
	else
	{
		GetDisplayName(navigateParams.pidl.Raw(), SHGDN_INFOLDER, name);
	}

	boost::replace_all(name, L"&", L"&&");

	TCITEM tcItem = {};
	tcItem.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
	tcItem.pszText = name.data();
	tcItem.iImage = tab.IsLocked() ? m_tabIconLockIndex : m_defaultFolderIconIndex;
	tcItem.lParam = tab.GetId();
	int insertedIndex = TabCtrl_InsertItem(m_hwnd, index, &tcItem);
	CHECK_NE(insertedIndex, -1);

	return insertedIndex;
}

bool TabContainerImpl::CloseTab(const Tab &tab)
{
	const int nTabs = GetNumTabs();

	if (nTabs == 1)
	{
		if (m_config->closeMainWindowOnTabClose)
		{
			m_browser->Close();
			return true;
		}
		else
		{
			return false;
		}
	}

	/* The tab is locked. Don't close it. */
	if (tab.GetLockState() == Tab::LockState::Locked
		|| tab.GetLockState() == Tab::LockState::AddressLocked)
	{
		return false;
	}

	m_app->GetTabEvents()->NotifyPreRemoval(tab, GetTabIndex(tab));

	RemoveTabFromControl(tab);

	auto dirMonitorId = tab.GetShellBrowserImpl()->GetDirMonitorId();

	if (dirMonitorId)
	{
		m_coreInterface->GetDirectoryMonitor()->StopDirectoryMonitor(*dirMonitorId);
	}

	// Taking ownership of the tab here will ensure it's still live when the observers are notified
	// below.
	auto itr = m_tabs.find(tab.GetId());
	CHECK(itr != m_tabs.end());
	auto ownedTab = std::move(*itr);

	m_tabs.erase(itr);

	OnTabRemoved(tab);

	return true;
}

void TabContainerImpl::RemoveTabFromControl(const Tab &tab)
{
	m_tabSelectionHistory.erase(
		std::remove(m_tabSelectionHistory.begin(), m_tabSelectionHistory.end(), tab.GetId()),
		m_tabSelectionHistory.end());

	const int index = GetTabIndex(tab);

	if (IsTabSelected(tab))
	{
		int newIndex;

		/* If there was a previously active tab, the focus
		should be switched back to it. */
		if (!m_tabSelectionHistory.empty())
		{
			const int lastTabId = m_tabSelectionHistory.back();
			m_tabSelectionHistory.pop_back();

			const Tab &lastTab = GetTab(lastTabId);
			newIndex = GetTabIndex(lastTab);
		}
		else
		{
			// If the last tab in the control is what's being closed,
			// the tab before it will be selected.
			if (index == (GetNumTabs() - 1))
			{
				newIndex = index - 1;
			}
			else
			{
				newIndex = index + 1;
			}
		}

		SelectTabAtIndex(newIndex);

		// This is somewhat hacky. Switching the tab will cause the
		// previously selected tab (i.e. the tab that's about to be
		// closed) to be added to the history list. That's not
		// desirable, so the last entry will be removed here.
		m_tabSelectionHistory.pop_back();
	}

	TCITEM tcItemRemoved;
	tcItemRemoved.mask = TCIF_IMAGE;
	TabCtrl_GetItem(m_hwnd, index, &tcItemRemoved);

	TabCtrl_DeleteItem(m_hwnd, index);

	if (!IsDefaultIcon(tcItemRemoved.iImage))
	{
		TabCtrl_RemoveImage(m_hwnd, tcItemRemoved.iImage);
	}
}

Tab &TabContainerImpl::GetTab(int tabId) const
{
	auto *tab = GetTabOptional(tabId);
	CHECK(tab);
	return *tab;
}

Tab *TabContainerImpl::GetTabOptional(int tabId) const
{
	auto itr = m_tabs.find(tabId);

	if (itr == m_tabs.end())
	{
		return nullptr;
	}

	return itr->second.get();
}

void TabContainerImpl::SelectTab(const Tab &tab)
{
	int index = GetTabIndex(tab);
	SelectTabAtIndex(index);
}

void TabContainerImpl::SelectAdjacentTab(BOOL bNextTab)
{
	int nTabs = GetNumTabs();
	int newIndex = GetSelectedTabIndex();

	if (bNextTab)
	{
		/* If this is the last tab in the order,
		wrap the selection back to the start. */
		if (newIndex == (nTabs - 1))
		{
			newIndex = 0;
		}
		else
		{
			newIndex++;
		}
	}
	else
	{
		/* If this is the first tab in the order,
		wrap the selection back to the end. */
		if (newIndex == 0)
		{
			newIndex = nTabs - 1;
		}
		else
		{
			newIndex--;
		}
	}

	SelectTabAtIndex(newIndex);
}

void TabContainerImpl::SelectTabAtIndex(int index)
{
	CHECK(index >= 0 && index < GetNumTabs());

	TabCtrl_SetCurSel(m_hwnd, index);

	OnTabSelected(GetTabByIndex(index));
}

Tab &TabContainerImpl::GetSelectedTab() const
{
	int index = GetSelectedTabIndex();
	return GetTabByIndex(index);
}

int TabContainerImpl::GetSelectedTabIndex() const
{
	int index = TabCtrl_GetCurSel(m_hwnd);
	CHECK_NE(index, -1) << "No selected tab";
	return index;
}

std::optional<int> TabContainerImpl::GetSelectedTabIndexOptional() const
{
	int index = TabCtrl_GetCurSel(m_hwnd);

	if (index == -1)
	{
		return std::nullopt;
	}

	return index;
}

bool TabContainerImpl::IsTabSelected(const Tab &tab) const
{
	const Tab &selectedTab = GetSelectedTab();
	return tab.GetId() == selectedTab.GetId();
}

Tab &TabContainerImpl::GetTabByIndex(int index) const
{
	TCITEM tcItem;
	tcItem.mask = TCIF_PARAM;
	BOOL res = TabCtrl_GetItem(m_hwnd, index, &tcItem);
	CHECK(res) << "Tab lookup failed for tab at index " << index;

	return GetTab(static_cast<int>(tcItem.lParam));
}

int TabContainerImpl::GetTabIndex(const Tab &tab) const
{
	int numTabs = TabCtrl_GetItemCount(m_hwnd);

	for (int i = 0; i < numTabs; i++)
	{
		TCITEM tcItem;
		tcItem.mask = TCIF_PARAM;
		BOOL res = TabCtrl_GetItem(m_hwnd, i, &tcItem);

		if (res && (tcItem.lParam == tab.GetId()))
		{
			return i;
		}
	}

	// All internal tab objects should have an index.
	LOG(FATAL) << "Couldn't determine index for tab";
}

int TabContainerImpl::GetNumTabs() const
{
	return static_cast<int>(m_tabs.size());
}

int TabContainerImpl::MoveTab(const Tab &tab, int newIndex)
{
	int index = GetTabIndex(tab);
	return TabCtrl_MoveItem(m_hwnd, index, newIndex);
}

std::unordered_map<int, std::unique_ptr<Tab>> &TabContainerImpl::GetTabs()
{
	return m_tabs;
}

const std::unordered_map<int, std::unique_ptr<Tab>> &TabContainerImpl::GetAllTabs() const
{
	return m_tabs;
}

std::vector<std::reference_wrapper<const Tab>> TabContainerImpl::GetAllTabsInOrder() const
{
	std::vector<std::reference_wrapper<const Tab>> sortedTabs;

	for (const auto &tab : m_tabs | boost::adaptors::map_values)
	{
		sortedTabs.emplace_back(*tab);
	}

	// The Tab class is non-copyable, so there are essentially two ways of
	// retrieving a sorted list of tabs, as far as I can tell:
	//
	// 1. The first is to maintain a sorted list of tabs while the program is
	// running. I generally don't think that's a good idea, since it would be
	// redundant (the tab control already stores that information) and it risks
	// possible issues (if the two sets get out of sync).
	//
	// 2. The second is to sort the tabs when needed. Because they're
	// non-copyable, that can't be done directly. std::reference_wrapper allows
	// it to be done relatively easily, though. Sorting a set of pointers would
	// accomplish the same thing.
	std::sort(sortedTabs.begin(), sortedTabs.end(), [this](const auto &tab1, const auto &tab2)
		{ return GetTabIndex(tab1.get()) < GetTabIndex(tab2.get()); });

	return sortedTabs;
}

void TabContainerImpl::DuplicateTab(const Tab &tab)
{
	auto folderSettings = tab.GetShellBrowserImpl()->GetFolderSettings();
	auto folderColumns = tab.GetShellBrowserImpl()->ExportAllColumns();
	auto navigateParams =
		NavigateParams::Normal(tab.GetShellBrowserImpl()->GetDirectoryIdl().get());
	CreateNewTab(navigateParams, {}, &folderSettings, &folderColumns);
}

void TabContainerImpl::MoveTabToNewWindow(const Tab &tab)
{
	tab.GetBrowser()->OpenItem(tab.GetShellBrowserImpl()->GetDirectoryIdl().get(), 
		OpenFolderDisposition::NewWindow);
	CloseTab(tab);
}

int TabContainerImpl::GetDropTargetItem(const POINT &pt)
{
	POINT ptClient = pt;
	BOOL res = ScreenToClient(m_hwnd, &ptClient);

	if (!res)
	{
		return -1;
	}

	TCHITTESTINFO hitTestInfo;
	hitTestInfo.pt = ptClient;
	return TabCtrl_HitTest(m_hwnd, &hitTestInfo);
}

unique_pidl_absolute TabContainerImpl::GetPidlForTargetItem(int targetItem)
{
	if (targetItem == -1)
	{
		return nullptr;
	}

	const auto &tab = GetTabByIndex(targetItem);
	return tab.GetShellBrowserImpl()->GetDirectoryIdl();
}

IUnknown *TabContainerImpl::GetSiteForTargetItem(PCIDLIST_ABSOLUTE targetItemPidl)
{
	UNREFERENCED_PARAMETER(targetItemPidl);

	return nullptr;
}

bool TabContainerImpl::IsTargetSourceOfDrop(int targetItem, IDataObject *dataObject)
{
	UNREFERENCED_PARAMETER(targetItem);
	UNREFERENCED_PARAMETER(dataObject);

	return false;
}

void TabContainerImpl::UpdateUiForDrop(int targetItem, const POINT &pt)
{
	UpdateUiForTargetItem(targetItem);
	ScrollTabControlForDrop(pt);
}

void TabContainerImpl::UpdateUiForTargetItem(int targetItem)
{
	if (!m_dropTargetContext)
	{
		m_dropTargetContext = DropTargetContext(&m_timerManager);
	}

	if (targetItem == -1)
	{
		m_dropTargetContext->switchTabTimer.Stop();
	}
	else if (m_dropTargetContext->targetIndex != targetItem)
	{
		m_dropTargetContext->switchTabTimer.Start(500ms,
			std::bind(&TabContainerImpl::OnDropSwitchTabTimer, this));
	}

	m_dropTargetContext->targetIndex = targetItem;
}

void TabContainerImpl::ScrollTabControlForDrop(const POINT &pt)
{
	POINT ptClient = pt;
	BOOL res = ScreenToClient(m_hwnd, &ptClient);

	if (!res)
	{
		return;
	}

	RECT rc;
	res = GetClientRect(m_hwnd, &rc);

	if (!res)
	{
		return;
	}

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hwnd);
	std::optional<ScrollDirection> scrollDirection;

	if (ptClient.x < MulDiv(DROP_SCROLL_MARGIN_X_96DPI, dpi, USER_DEFAULT_SCREEN_DPI))
	{
		scrollDirection = ScrollDirection::Left;
	}
	else if (ptClient.x
		> (rc.right - MulDiv(DROP_SCROLL_MARGIN_X_96DPI, dpi, USER_DEFAULT_SCREEN_DPI)))
	{
		scrollDirection = ScrollDirection::Right;
	}

	if (!scrollDirection)
	{
		m_dropTargetContext->scrollTimer.Stop();
		return;
	}

	if (!m_dropTargetContext->scrollDirection
		|| scrollDirection != *m_dropTargetContext->scrollDirection)
	{
		m_dropTargetContext->scrollDirection = scrollDirection;
		m_dropTargetContext->scrollTimer.Start(1s,
			std::bind(&TabContainerImpl::OnDropScrollTimer, this));
	}
}

void TabContainerImpl::ResetDropUiState()
{
	m_dropTargetContext.reset();
}

void TabContainerImpl::OnDropSwitchTabTimer()
{
	CHECK(m_dropTargetContext);
	CHECK_NE(m_dropTargetContext->targetIndex, -1);

	if (m_dropTargetContext->targetIndex != GetSelectedTabIndex())
	{
		SelectTabAtIndex(m_dropTargetContext->targetIndex);
	}
}

void TabContainerImpl::OnDropScrollTimer()
{
	CHECK(m_dropTargetContext);
	CHECK(m_dropTargetContext->scrollDirection);
	ScrollTabControl(*m_dropTargetContext->scrollDirection);

	m_dropTargetContext->scrollDirection.reset();
}

void TabContainerImpl::ScrollTabControl(ScrollDirection direction)
{
	HWND upDownControl = FindWindowEx(m_hwnd, nullptr, UPDOWN_CLASS, nullptr);

	// It's valid for the control not to exist if all the tabs fit within the window.
	if (!upDownControl)
	{
		return;
	}

	int lowerLimit;
	int upperLimit;
	SendMessage(upDownControl, UDM_GETRANGE32, reinterpret_cast<WPARAM>(&lowerLimit),
		reinterpret_cast<WPARAM>(&upperLimit));

	BOOL positionRetrieved;
	auto position =
		SendMessage(upDownControl, UDM_GETPOS32, 0, reinterpret_cast<LPARAM>(&positionRetrieved));

	if (!positionRetrieved)
	{
		return;
	}

	switch (direction)
	{
	case ScrollDirection::Left:
		position--;
		break;

	case ScrollDirection::Right:
		position++;
		break;
	}

	if (position < lowerLimit || position > upperLimit)
	{
		return;
	}

	SendMessage(m_hwnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, position), NULL);
}

void TabContainerImpl::OnFontUpdated()
{
	sizeUpdatedSignal.m_signal();
}

std::vector<TabStorageData> TabContainerImpl::GetStorageData() const
{
	std::vector<TabStorageData> tabListStorageData;

	for (auto tabRef : GetAllTabsInOrder())
	{
		const auto &tab = tabRef.get();
		tabListStorageData.push_back(tab.GetStorageData());
	}

	return tabListStorageData;
}
