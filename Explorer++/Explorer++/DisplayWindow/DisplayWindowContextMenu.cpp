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
	m_config(config)
{
	BuildMenu(resourceLoader);
}

void DisplayWindowContextMenu::BuildMenu(const ResourceLoader *resourceLoader)
{
	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_DISPLAY_WINDOW_CONTEXT_MENU_CHANGE_COLORS))
			.SetHelpText(
				resourceLoader->LoadString(IDS_DISPLAY_WINDOW_CONTEXT_MENU_CHANGE_COLORS_HELP_TEXT))
			.SetSelectedCallback([this] { OnChangeColors(); }));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_DISPLAY_WINDOW_CONTEXT_MENU_HIDE))
			.SetHelpText(resourceLoader->LoadString(IDS_DISPLAY_WINDOW_CONTEXT_MENU_HIDE_HELP_TEXT))
			.SetSelectedCallback([this] { m_config->showDisplayWindow = false; }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_DISPLAY_WINDOW_CONTEXT_MENU_POSITION_RIGHT))
			.SetHelpText(resourceLoader->LoadString(
				IDS_DISPLAY_WINDOW_CONTEXT_MENU_POSITION_RIGHT_HELP_TEXT))
			.SetCheckedCallback([this] { return m_config->displayWindowVertical.get(); })
			.SetSelectedCallback([this]
				{ m_config->displayWindowVertical = !m_config->displayWindowVertical.get(); }));
}

void DisplayWindowContextMenu::OnChangeColors()
{
	m_browser->GetCommandController()->ExecuteCommand(IDM_VIEW_CHANGEDISPLAYCOLOURS);
}
