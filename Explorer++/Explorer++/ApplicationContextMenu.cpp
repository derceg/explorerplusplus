// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationContextMenu.h"
#include "ApplicationModel.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include <wil/resource.h>

namespace Applications
{

ApplicationContextMenu::ApplicationContextMenu(ApplicationModel *model,
	ApplicationExecutor *applicationExecutor, CoreInterface *coreInterface) :
	m_model(model),
	m_resourceInstance(coreInterface->GetResourceInstance()),
	m_controller(applicationExecutor, coreInterface)
{
}

void ApplicationContextMenu::ShowMenu(HWND parentWindow, Application *application,
	const POINT &ptScreen)
{
	wil::unique_hmenu parentMenu(
		LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_APPLICATIONTOOLBAR_MENU)));
	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	UINT menuItemId = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RETURNCMD, ptScreen.x, ptScreen.y, 0,
		parentWindow, nullptr);

	if (menuItemId == 0)
	{
		return;
	}

	auto index = m_model->GetItemIndex(application);

	m_controller.OnMenuItemSelected(menuItemId, m_model, application, index + 1, parentWindow);
}

}
