// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationToolbarView.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceHelper.h"

namespace Applications
{

ApplicationToolbarView *ApplicationToolbarView::Create(HWND parent, CoreInterface *coreInterface,
	ApplicationModel *model)
{
	return new ApplicationToolbarView(parent, coreInterface, model);
}

ApplicationToolbarView::ApplicationToolbarView(HWND parent, CoreInterface *coreInterface,
	ApplicationModel *model) :
	ToolbarView(parent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_DOUBLEBUFFER
			| TBSTYLE_EX_HIDECLIPPEDBUTTONS),
	m_resourceModule(coreInterface->GetResourceModule()),
	m_contextMenu(model, coreInterface)
{
	SetupSmallShellImageList();

	m_connections.push_back(coreInterface->AddToolbarContextMenuObserver(
		std::bind_front(&ApplicationToolbarView::OnToolbarContextMenuPreShow, this)));
}

void ApplicationToolbarView::ShowContextMenu(Application *application, const POINT &ptClient)
{
	POINT ptScreen = ptClient;
	BOOL res = ClientToScreen(m_hwnd, &ptScreen);

	if (!res)
	{
		return;
	}

	m_contextMenu.ShowMenu(m_hwnd, application, ptScreen);
}

void ApplicationToolbarView::OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow,
	const POINT &pt)
{
	UNREFERENCED_PARAMETER(pt);

	if (sourceWindow != m_hwnd)
	{
		return;
	}

	std::wstring newText = ResourceHelper::LoadString(m_resourceModule, IDS_APPLICATIONBUTTON_NEW);

	// TODO: This menu item is currently handled in MainWndSwitch.cpp, but should ideally be handled
	// by this class, since it adds the menu item.
	MENUITEMINFO mii = {};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.dwTypeData = newText.data();
	mii.wID = IDM_APP_NEW;
	InsertMenuItem(menu, IDM_TOOLBARS_CUSTOMIZE, FALSE, &mii);
}

}
