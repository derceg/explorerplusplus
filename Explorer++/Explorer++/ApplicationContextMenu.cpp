// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationContextMenu.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"

namespace Applications
{

ApplicationContextMenu::ApplicationContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, ApplicationModel *model, Application *application,
	ApplicationExecutor *applicationExecutor, const ResourceLoader *resourceLoader,
	CoreInterface *coreInterface, ThemeManager *themeManager) :
	MenuBase(menuView, acceleratorManager),
	m_controller(model, application, applicationExecutor, resourceLoader, coreInterface,
		themeManager)
{
	BuildMenu(resourceLoader);

	m_connections.push_back(m_menuView->AddItemSelectedObserver(
		std::bind(&ApplicationContextMenu::OnMenuItemSelected, this, std::placeholders::_1)));
}

void ApplicationContextMenu::BuildMenu(const ResourceLoader *resourceLoader)
{
	m_menuView->AppendItem(IDM_APPLICATION_CONTEXT_MENU_OPEN,
		resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_OPEN), {},
		resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_OPEN_HELP_TEXT));
	m_menuView->AppendSeparator();
	m_menuView->AppendItem(IDM_APPLICATION_CONTEXT_MENU_NEW,
		resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_NEW), {},
		resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_NEW_HELP_TEXT));
	m_menuView->AppendSeparator();
	m_menuView->AppendItem(IDM_APPLICATION_CONTEXT_MENU_DELETE,
		resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_DELETE), {},
		resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_DELETE_HELP_TEXT));
	m_menuView->AppendItem(IDM_APPLICATION_CONTEXT_MENU_PROPERTIES,
		resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_PROPERTIES), {},
		resourceLoader->LoadString(IDS_APPLICATION_CONTEXT_MENU_PROPERTIES_HELP_TEXT));
}

void ApplicationContextMenu::OnMenuItemSelected(UINT menuItemId)
{
	m_controller.OnMenuItemSelected(menuItemId);
}

}
