// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DisplayWindowContextMenu.h"
#include "BrowserCommandController.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"

DisplayWindowContextMenu::DisplayWindowContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, BrowserWindow *browser, Config *config,
	const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_browser(browser),
	m_config(config),
	m_resourceLoader(resourceLoader)
{
	BuildMenu();
}

void DisplayWindowContextMenu::BuildMenu()
{
	m_menuView->AppendItem(this, IDM_DISPLAY_WINDOW_CONTEXT_MENU_CHANGE_COLORS,
		m_resourceLoader->LoadString(IDS_DISPLAY_WINDOW_CONTEXT_MENU_CHANGE_COLORS), {},
		m_resourceLoader->LoadString(IDS_DISPLAY_WINDOW_CONTEXT_MENU_CHANGE_COLORS_HELP_TEXT));

	m_menuView->AppendSeparator();

	m_menuView->AppendItem(this, IDM_DISPLAY_WINDOW_CONTEXT_MENU_HIDE,
		m_resourceLoader->LoadString(IDS_DISPLAY_WINDOW_CONTEXT_MENU_HIDE), {},
		m_resourceLoader->LoadString(IDS_DISPLAY_WINDOW_CONTEXT_MENU_HIDE_HELP_TEXT));
	m_menuView->AppendItem(this, IDM_DISPLAY_WINDOW_CONTEXT_MENU_POSITION_RIGHT,
		m_resourceLoader->LoadString(IDS_DISPLAY_WINDOW_CONTEXT_MENU_POSITION_RIGHT), {},
		m_resourceLoader->LoadString(IDS_DISPLAY_WINDOW_CONTEXT_MENU_POSITION_RIGHT_HELP_TEXT));

	m_menuView->CheckItem(IDM_DISPLAY_WINDOW_CONTEXT_MENU_POSITION_RIGHT,
		m_config->displayWindowVertical.get());
}

void DisplayWindowContextMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	switch (id)
	{
	case IDM_DISPLAY_WINDOW_CONTEXT_MENU_CHANGE_COLORS:
		m_browser->GetCommandController()->ExecuteCommand(IDM_VIEW_CHANGEDISPLAYCOLOURS);
		break;

	case IDM_DISPLAY_WINDOW_CONTEXT_MENU_HIDE:
		m_config->showDisplayWindow = false;
		break;

	case IDM_DISPLAY_WINDOW_CONTEXT_MENU_POSITION_RIGHT:
		m_config->displayWindowVertical = !m_config->displayWindowVertical.get();
		break;

	default:
		DCHECK(false);
		break;
	}
}
