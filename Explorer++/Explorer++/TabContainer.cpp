// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContainer.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "MainImages.h"
#include "MainResource.h"
#include "RenameTabDialog.h"
#include "ResourceHelper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/MenuWrapper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/TabHelper.h"
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>

const std::map<UINT, int> TAB_RIGHT_CLICK_MENU_IMAGE_MAPPINGS = {
	{ IDM_FILE_NEWTAB, SHELLIMAGES_NEWTAB },
	{ IDM_TAB_REFRESH, SHELLIMAGES_REFRESH }
};

CTabContainer::CTabContainer(HWND hTabCtrl, std::unordered_map<int, Tab> *tabInfo, TabContainerInterface *tabContainer,
	TabInterface *tabInterface, IExplorerplusplus *expp, HINSTANCE instance, std::shared_ptr<Config> config) :
	m_hTabCtrl(hTabCtrl),
	m_tabInfo(tabInfo),
	m_tabContainer(tabContainer),
	m_tabInterface(tabInterface),
	m_expp(expp),
	m_instance(instance),
	m_config(config),
	m_bTabBeenDragged(FALSE)
{
	SetWindowSubclass(hTabCtrl, WndProcStub, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));
	SetWindowSubclass(GetParent(hTabCtrl), ParentWndProcStub, PARENT_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));

	m_tabCreatedConnection = m_tabContainer->AddTabCreatedObserver(boost::bind(&CTabContainer::OnTabCreated, this, _1, _2));
	m_tabRemovedConnection = m_tabContainer->AddTabRemovedObserver(boost::bind(&CTabContainer::OnTabRemoved, this, _1));

	m_navigationCompletedConnection = m_tabContainer->AddNavigationCompletedObserver(boost::bind(&CTabContainer::OnNavigationCompleted, this, _1));
	m_tabUpdatedConnection = m_tabContainer->AddTabUpdatedObserver(boost::bind(&CTabContainer::OnTabUpdated, this, _1, _2));

	m_alwaysShowTabBarConnection = m_config->alwaysShowTabBar.addObserver(boost::bind(&CTabContainer::OnAlwaysShowTabBarUpdated, this, _1));
}

CTabContainer::~CTabContainer()
{
	RemoveWindowSubclass(m_hTabCtrl, WndProcStub, SUBCLASS_ID);
	RemoveWindowSubclass(GetParent(m_hTabCtrl), ParentWndProcStub, PARENT_SUBCLASS_ID);

	m_tabCreatedConnection.disconnect();
	m_tabRemovedConnection.disconnect();

	m_navigationCompletedConnection.disconnect();
	m_tabUpdatedConnection.disconnect();

	m_alwaysShowTabBarConnection.disconnect();
}

LRESULT CALLBACK CTabContainer::WndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CTabContainer *tabContainer = reinterpret_cast<CTabContainer *>(dwRefData);
	return tabContainer->WndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CTabContainer::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
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
			if ((HWND)lParam != hwnd)
			{
				ReleaseCapture();
			}

			m_bTabBeenDragged = FALSE;
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void CTabContainer::OnTabCtrlLButtonDown(POINT *pt)
{
	TCHITTESTINFO info;
	info.pt = *pt;
	int ItemNum = TabCtrl_HitTest(m_hTabCtrl, &info);

	if (info.flags != TCHT_NOWHERE)
	{
		/* Save the bounds of the dragged tab. */
		TabCtrl_GetItemRect(m_hTabCtrl, ItemNum, &m_rcDraggedTab);

		/* Capture mouse movement exclusively until
		the mouse button is released. */
		SetCapture(m_hTabCtrl);

		m_bTabBeenDragged = TRUE;
		m_draggedTabStartIndex = ItemNum;
		m_draggedTabEndIndex = ItemNum;
	}
}

void CTabContainer::OnTabCtrlLButtonUp(void)
{
	if (!m_bTabBeenDragged)
	{
		return;
	}

	ReleaseCapture();

	m_bTabBeenDragged = FALSE;

	if (m_draggedTabEndIndex != m_draggedTabStartIndex)
	{
		const Tab &tab = m_tabContainer->GetTabByIndex(m_draggedTabEndIndex);
		m_tabMovedSignal(tab, m_draggedTabStartIndex, m_draggedTabEndIndex);
	}
}

void CTabContainer::OnTabCtrlMouseMove(POINT *pt)
{
	if (!m_bTabBeenDragged)
	{
		return;
	}

	/* Dragged tab. */
	int iSelected = TabCtrl_GetCurFocus(m_hTabCtrl);

	TCHITTESTINFO HitTestInfo;
	HitTestInfo.pt = *pt;
	int iSwap = TabCtrl_HitTest(m_hTabCtrl, &HitTestInfo);

	/* Check:
	- If the cursor is over an item.
	- If the cursor is not over the dragged item itself.
	- If the cursor has passed to the left of the dragged tab, or
	- If the cursor has passed to the right of the dragged tab. */
	if (HitTestInfo.flags != TCHT_NOWHERE &&
		iSwap != iSelected &&
		(pt->x < m_rcDraggedTab.left ||
			pt->x > m_rcDraggedTab.right))
	{
		RECT rcSwap;

		TabCtrl_GetItemRect(m_hTabCtrl, iSwap, &rcSwap);

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
		TabCtrl_SwapItems(m_hTabCtrl, iSelected, iSwap);

		/* The index of the selected tab has now changed
		(but the actual tab/browser selected remains the
		same). */
		TabCtrl_SetCurFocus(m_hTabCtrl, iSwap);

		m_draggedTabEndIndex = iSwap;
	}
}

void CTabContainer::OnTabCtrlMButtonUp(POINT *pt)
{
	TCHITTESTINFO htInfo;
	htInfo.pt = *pt;

	/* Find the tab that the click occurred over. */
	int iTabHit = TabCtrl_HitTest(m_hTabCtrl, &htInfo);

	if (iTabHit != -1)
	{
		const Tab &tab = m_tabContainer->GetTabByIndex(iTabHit);
		m_tabContainer->CloseTab(tab);
	}
}

void CTabContainer::OnTabCtrlRButtonUp(POINT *pt)
{
	TCHITTESTINFO tcHitTest;
	tcHitTest.pt = *pt;
	const int tabHitIndex = TabCtrl_HitTest(m_hTabCtrl, &tcHitTest);

	if (tcHitTest.flags == TCHT_NOWHERE)
	{
		return;
	}

	POINT ptCopy = *pt;
	BOOL res = ClientToScreen(m_hTabCtrl, &ptCopy);

	if (!res)
	{
		return;
	}

	Tab &tab = m_tabContainer->GetTabByIndex(tabHitIndex);

	CreateTabContextMenu(tab, ptCopy);
}

void CTabContainer::CreateTabContextMenu(Tab &tab, const POINT &pt)
{
	auto parentMenu = MenuPtr(LoadMenu(m_instance, MAKEINTRESOURCE(IDR_TAB_RCLICK)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	std::vector<HBitmapPtr> menuImages;
	AddImagesToTabContextMenu(menu, menuImages);

	lCheckMenuItem(menu, IDM_TAB_LOCKTAB, tab.GetLocked());
	lCheckMenuItem(menu, IDM_TAB_LOCKTABANDADDRESS, tab.GetAddressLocked());
	lEnableMenuItem(menu, IDM_TAB_CLOSETAB, !(tab.GetLocked() || tab.GetAddressLocked()));

	UINT Command = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERTICAL | TPM_RETURNCMD,
		pt.x, pt.y, 0, m_hTabCtrl, nullptr);

	ProcessTabCommand(Command, tab);
}

void CTabContainer::AddImagesToTabContextMenu(HMENU menu, std::vector<HBitmapPtr> &menuImages)
{
	HImageListPtr imageList = GetShellImageList();

	if (!imageList)
	{
		return;
	}

	for (auto mapping : TAB_RIGHT_CLICK_MENU_IMAGE_MAPPINGS)
	{
		SetMenuItemImageFromImageList(menu, mapping.first, imageList.get(), mapping.second, menuImages);
	}
}

void CTabContainer::ProcessTabCommand(UINT uMenuID, Tab &tab)
{
	switch (uMenuID)
	{
		case IDM_FILE_NEWTAB:
			/* Send the resulting command back to the main window for processing. */
			SendMessage(m_expp->GetMainWindow(), WM_COMMAND, MAKEWPARAM(uMenuID, 0), 0);
			break;

		case IDM_TAB_DUPLICATETAB:
			m_tabContainer->DuplicateTab(tab);
			break;

		case IDM_TAB_OPENPARENTINNEWTAB:
			OnOpenParentInNewTab(tab);
			break;

		case IDM_TAB_REFRESH:
			m_tabInterface->RefreshTab(tab);
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
			m_tabContainer->CloseTab(tab);
			break;

		case IDM_TAB_CLOSEOTHERTABS:
			OnCloseOtherTabs(m_tabContainer->GetTabIndex(tab));
			break;

		case IDM_TAB_CLOSETABSTORIGHT:
			OnCloseTabsToRight(m_tabContainer->GetTabIndex(tab));
			break;
	}
}

void CTabContainer::OnOpenParentInNewTab(const Tab &tab)
{
	LPITEMIDLIST pidlCurrent = tab.GetShellBrowser()->QueryCurrentDirectoryIdl();

	LPITEMIDLIST pidlParent = NULL;
	HRESULT hr = GetVirtualParentPath(pidlCurrent, &pidlParent);

	if (SUCCEEDED(hr))
	{
		m_tabContainer->CreateNewTab(pidlParent, TabSettings(_selected = true));
		CoTaskMemFree(pidlParent);
	}

	CoTaskMemFree(pidlCurrent);
}

void CTabContainer::OnRefreshAllTabs()
{
	for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
	{
		m_tabInterface->RefreshTab(tab);
	}
}

void CTabContainer::OnRenameTab(const Tab &tab)
{
	CRenameTabDialog RenameTabDialog(m_instance, IDD_RENAMETAB, m_expp->GetMainWindow(), tab.GetId(), m_expp, m_tabContainer, m_tabInterface);
	RenameTabDialog.ShowModalDialog();
}

void CTabContainer::OnLockTab(Tab &tab)
{
	tab.SetLocked(!tab.GetLocked());
}

void CTabContainer::OnLockTabAndAddress(Tab &tab)
{
	tab.SetAddressLocked(!tab.GetAddressLocked());
}

void CTabContainer::OnCloseOtherTabs(int index)
{
	const int nTabs = m_tabContainer->GetNumTabs();

	/* Close all tabs except the
	specified one. */
	for (int i = nTabs - 1; i >= 0; i--)
	{
		if (i != index)
		{
			const Tab &tab = m_tabContainer->GetTabByIndex(i);
			m_tabContainer->CloseTab(tab);
		}
	}
}

void CTabContainer::OnCloseTabsToRight(int index)
{
	int nTabs = m_tabContainer->GetNumTabs();

	for (int i = nTabs - 1; i > index; i--)
	{
		const Tab &currentTab = m_tabContainer->GetTabByIndex(i);
		m_tabContainer->CloseTab(currentTab);
	}
}

boost::signals2::connection CTabContainer::AddTabMovedObserver(const TabMovedSignal::slot_type &observer)
{
	return m_tabMovedSignal.connect(observer);
}

LRESULT CALLBACK CTabContainer::ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CTabContainer *tabContainer = reinterpret_cast<CTabContainer *>(dwRefData);
	return tabContainer->ParentWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CTabContainer::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code)
		{
		case TTN_GETDISPINFO:
			OnGetDispInfo(reinterpret_cast<NMTTDISPINFO *>(lParam));
			break;
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void CTabContainer::OnGetDispInfo(NMTTDISPINFO *dispInfo)
{
	HWND toolTipControl = TabCtrl_GetToolTips(m_hTabCtrl);

	if (dispInfo->hdr.hwndFrom != toolTipControl)
	{
		return;
	}

	static TCHAR tabToolTip[512];

	const Tab &tab = m_tabContainer->GetTabByIndex(static_cast<int>(dispInfo->hdr.idFrom));

	PIDLPointer pidlDirectory(tab.GetShellBrowser()->QueryCurrentDirectoryIdl());
	auto path = GetFolderPathForDisplay(pidlDirectory.get());

	if (!path)
	{
		return;
	}

	StringCchCopy(tabToolTip, SIZEOF_ARRAY(tabToolTip), path->c_str());

	dispInfo->lpszText = tabToolTip;
}

int CTabContainer::GetSelection()
{
	int Index = TabCtrl_GetCurSel(m_hTabCtrl);
	assert(Index != -1);

	return Index;
}

void CTabContainer::OnTabCreated(int tabId, BOOL switchToNewTab)
{
	UNREFERENCED_PARAMETER(tabId);
	UNREFERENCED_PARAMETER(switchToNewTab);

	if (!m_config->alwaysShowTabBar.get() &&
		(m_tabContainer->GetNumTabs() > 1))
	{
		m_expp->ShowTabBar();
	}
}

void CTabContainer::OnTabRemoved(int tabId)
{
	UNREFERENCED_PARAMETER(tabId);

	if (!m_config->alwaysShowTabBar.get() &&
		(m_tabContainer->GetNumTabs() == 1))
	{
		m_expp->HideTabBar();
	}
}

void CTabContainer::OnAlwaysShowTabBarUpdated(BOOL newValue)
{
	if (newValue)
	{
		m_expp->ShowTabBar();
	}
	else
	{
		if (m_tabContainer->GetNumTabs() > 1)
		{
			m_expp->ShowTabBar();
		}
		else
		{
			m_expp->HideTabBar();
		}
	}
}

void CTabContainer::OnNavigationCompleted(const Tab &tab)
{
	UpdateTabNameInWindow(tab);
	SetTabIcon(tab);
}

void CTabContainer::OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
{
	switch (propertyType)
	{
	case Tab::PropertyType::LOCKED:
	case Tab::PropertyType::ADDRESS_LOCKED:
		SetTabIcon(tab);
		break;

	case Tab::PropertyType::NAME:
		UpdateTabNameInWindow(tab);
		break;
	}
}

void CTabContainer::UpdateTabNameInWindow(const Tab &tab)
{
	std::wstring name = tab.GetName();
	boost::replace_all(name, L"&", L"&&");

	int index = m_tabContainer->GetTabIndex(tab);
	TabCtrl_SetItemText(m_hTabCtrl, index, name.c_str());
}

/* Sets a tabs icon. Normally, this icon
is the folders icon, however if the tab
is locked, the icon will be a lock. */
void CTabContainer::SetTabIcon(const Tab &tab)
{
	TCITEM			tcItem;
	SHFILEINFO		shfi;
	ICONINFO		IconInfo;
	int				iImage;
	int				iRemoveImage;

	/* If the tab is locked, use a lock icon. */
	if (tab.GetAddressLocked() || tab.GetLocked())
	{
		iImage = TAB_ICON_LOCK_INDEX;
	}
	else
	{
		PIDLPointer pidlDirectory(tab.GetShellBrowser()->QueryCurrentDirectoryIdl());

		SHGetFileInfo((LPCTSTR)pidlDirectory.get(), 0, &shfi, sizeof(shfi),
			SHGFI_PIDL | SHGFI_ICON | SHGFI_SMALLICON);

		GetIconInfo(shfi.hIcon, &IconInfo);
		iImage = ImageList_Add(TabCtrl_GetImageList(m_hTabCtrl),
			IconInfo.hbmColor, IconInfo.hbmMask);

		DeleteObject(IconInfo.hbmColor);
		DeleteObject(IconInfo.hbmMask);
		DestroyIcon(shfi.hIcon);
	}

	int index = m_tabContainer->GetTabIndex(tab);

	/* Get the index of the current image. This image
	will be removed after the new image is set. */
	tcItem.mask = TCIF_IMAGE;
	TabCtrl_GetItem(m_hTabCtrl, index, &tcItem);

	iRemoveImage = tcItem.iImage;

	/* Set the new image. */
	tcItem.mask = TCIF_IMAGE;
	tcItem.iImage = iImage;
	TabCtrl_SetItem(m_hTabCtrl, index, &tcItem);

	if (iRemoveImage != TAB_ICON_LOCK_INDEX)
	{
		/* Remove the old image. */
		TabCtrl_RemoveImage(m_hTabCtrl, iRemoveImage);
	}
}