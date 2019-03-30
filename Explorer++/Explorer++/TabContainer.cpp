// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContainer.h"
#include "Explorer++_internal.h"
#include "../Helper/TabHelper.h"
#include <boost/algorithm/string.hpp>

CTabContainer::CTabContainer(HWND hTabCtrl, std::unordered_map<int, Tab> *tabInfo, TabContainerInterface *tabContainer) :
	m_hTabCtrl(hTabCtrl),
	m_tabInfo(tabInfo),
	m_tabContainer(tabContainer)
{
	SetWindowSubclass(GetParent(hTabCtrl), ParentWndProcStub, PARENT_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this));

	m_navigationCompletedConnection = m_tabContainer->AddNavigationCompletedObserver(boost::bind(&CTabContainer::OnNavigationCompleted, this, _1));
	m_tabUpdatedConnection = m_tabContainer->AddTabUpdatedObserver(boost::bind(&CTabContainer::OnTabUpdated, this, _1, _2));
}

CTabContainer::~CTabContainer()
{
	RemoveWindowSubclass(GetParent(m_hTabCtrl), ParentWndProcStub, PARENT_SUBCLASS_ID);

	m_navigationCompletedConnection.disconnect();
	m_tabUpdatedConnection.disconnect();
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
	static TCHAR szTabToolTip[512];

	HWND toolTipControl = TabCtrl_GetToolTips(m_hTabCtrl);

	if (dispInfo->hdr.hwndFrom == toolTipControl)
	{
		const Tab &tab = m_tabContainer->GetTabByIndex(static_cast<int>(dispInfo->hdr.idFrom));
		tab.GetShellBrowser()->QueryCurrentDirectory(SIZEOF_ARRAY(szTabToolTip),
			szTabToolTip);

		dispInfo->lpszText = szTabToolTip;
	}
}

int CTabContainer::GetSelection()
{
	int Index = TabCtrl_GetCurSel(m_hTabCtrl);
	assert(Index != -1);

	return Index;
}

CShellBrowser *CTabContainer::GetBrowserForTab(int Index)
{
	TCITEM tcItem;
	tcItem.mask = TCIF_PARAM;
	BOOL res = TabCtrl_GetItem(m_hTabCtrl,Index,&tcItem);
	assert(res);

	if(!res)
	{
		return NULL;
	}

	return m_tabInfo->at(static_cast<int>(tcItem.lParam)).GetShellBrowser();
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