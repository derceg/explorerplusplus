// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DrivesToolbarView.h"
#include "Config.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "TabContainer.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/ShellHelper.h"
#include <ShlObj.h>
#include <Shlwapi.h>

DrivesToolbarView *DrivesToolbarView::Create(HWND parent, CoreInterface *coreInterface,
	HINSTANCE instance)
{
	return new DrivesToolbarView(parent, coreInterface, instance);
}

DrivesToolbarView::DrivesToolbarView(HWND parent, CoreInterface *coreInterface,
	HINSTANCE instance) :
	ToolbarView(parent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE,
		TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS),
	m_coreInterface(coreInterface),
	m_instance(instance)
{
	SetupSmallShellImageList();
}

void DrivesToolbarView::ShowContextMenu(const std::wstring &drivePath, const POINT &ptClient,
	bool showExtended)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(drivePath.c_str(), nullptr, wil::out_param(pidl), 0, nullptr);

	if (FAILED(hr))
	{
		return;
	}

	POINT ptScreen = ptClient;
	ClientToScreen(m_hwnd, &ptScreen);

	unique_pidl_child child(ILCloneChild(ILFindLastID(pidl.get())));

	[[maybe_unused]] BOOL res = ILRemoveLastID(pidl.get());
	assert(res);

	FileContextMenuManager contextMenuManager(m_hwnd, pidl.get(), { child.get() });

	contextMenuManager.ShowMenu(this, MIN_SHELL_MENU_ID, MAX_SHELL_MENU_ID, &ptScreen,
		m_coreInterface->GetStatusBar(), NULL, FALSE, showExtended);
}

void DrivesToolbarView::UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, IContextMenu *contextMenu,
	HMENU hMenu)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(dwData);
	UNREFERENCED_PARAMETER(contextMenu);

	std::wstring openInNewTabText =
		ResourceHelper::LoadString(m_instance, IDS_GENERAL_OPEN_IN_NEW_TAB);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_ID;
	mii.wID = MENU_ID_OPEN_IN_NEW_TAB;
	mii.dwTypeData = openInNewTabText.data();
	InsertMenuItem(hMenu, 1, TRUE, &mii);
}

BOOL DrivesToolbarView::HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd)
{
	UNREFERENCED_PARAMETER(dwData);

	if (StrCmpI(szCmd, _T("open")) == 0)
	{
		assert(pidlItems.size() == 1);

		unique_pidl_absolute pidl(ILCombine(pidlParent, pidlItems[0]));
		m_coreInterface->OpenItem(pidl.get());
		return TRUE;
	}

	return FALSE;
}

void DrivesToolbarView::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, int iCmd)
{
	UNREFERENCED_PARAMETER(pidlItems);

	switch (iCmd)
	{
	case MENU_ID_OPEN_IN_NEW_TAB:
	{
		assert(pidlItems.size() == 1);

		unique_pidl_absolute pidl(ILCombine(pidlParent, pidlItems[0]));
		m_coreInterface->GetTabContainer()->CreateNewTab(pidl.get(),
			TabSettings(_selected = m_coreInterface->GetConfig()->openTabsInForeground));
	}
	break;
	}
}
