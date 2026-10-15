// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolsMenu.h"
#include "BrowserWindow.h"
#include "FeatureList.h"
#include "Icon.h"
#include "MainResource.h"
#include "MenuView.h"
#include "Plugins/PluginItemsMenu.h"
#include "Plugins/PluginMenuItemsRegistry.h"
#include "ResourceIconModel.h"
#include "ResourceLoader.h"

ToolsMenu::ToolsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	BrowserWindow *browser, const FeatureList *featureList,
	Plugins::PluginMenuItemsRegistry *pluginMenuItemsRegistry,
	const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_browser(browser),
	m_featureList(featureList),
	m_pluginMenuItemsRegistry(pluginMenuItemsRegistry),
	m_resourceLoader(resourceLoader)
{
	BuildMenu();

	m_connections.push_back(
		m_pluginMenuItemsRegistry->itemsChangedSignal.AddObserver([this] { RebuildMenu(); }));
}

void ToolsMenu::RebuildMenu()
{
	m_rootMenuView->ClearMenu();
	m_pluginsMenu.reset();
	BuildMenu();
}

void ToolsMenu::BuildMenu()
{
	m_rootMenuView->AppendItem(MenuItemParams(m_resourceLoader->LoadString(IDS_TOOLS_SEARCH))
			.SetIconModel(std::make_unique<ResourceIconModel>(Icon::Search, IconSize::Small,
				m_resourceLoader))
			.SetHelpText(m_resourceLoader->LoadString(IDS_TOOLS_SEARCH_HELP_TEXT))
			.SetAcceleratorText(GetAcceleratorTextForId(IDM_TOOLS_SEARCH))
			.SetSelectedCallback([this] { SendCommandToBrowser(IDM_TOOLS_SEARCH); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_TOOLS_CUSTOMIZECOLORS))
			.SetIconModel(std::make_unique<ResourceIconModel>(Icon::CustomizeColors,
				IconSize::Small, m_resourceLoader))
			.SetHelpText(m_resourceLoader->LoadString(IDS_TOOLS_CUSTOMIZECOLORS_HELP_TEXT))
			.SetAcceleratorText(GetAcceleratorTextForId(IDM_TOOLS_CUSTOMIZECOLORS))
			.SetSelectedCallback([this] { SendCommandToBrowser(IDM_TOOLS_CUSTOMIZECOLORS); }));

	m_rootMenuView->AppendSeparator();

	if (m_featureList->IsEnabled(Feature::Plugins))
	{
		m_rootMenuView->AppendItem(MenuItemParams(m_resourceLoader->LoadString(IDS_TOOLS_RUNSCRIPT))
				.SetHelpText(m_resourceLoader->LoadString(IDS_TOOLS_RUNSCRIPT_HELP_TEXT))
				.SetAcceleratorText(GetAcceleratorTextForId(IDM_TOOLS_RUNSCRIPT))
				.SetSelectedCallback([this] { SendCommandToBrowser(IDM_TOOLS_RUNSCRIPT); }));

		m_pluginsMenu = std::make_unique<Plugins::PluginItemsMenu>(m_rootMenuView,
			m_acceleratorManager, m_pluginMenuItemsRegistry);
	}

	m_rootMenuView->AppendItem(MenuItemParams(m_resourceLoader->LoadString(IDS_TOOLS_OPTIONS))
			.SetIconModel(std::make_unique<ResourceIconModel>(Icon::Options, IconSize::Small,
				m_resourceLoader))
			.SetHelpText(m_resourceLoader->LoadString(IDS_TOOLS_OPTIONS_HELP_TEXT))
			.SetAcceleratorText(GetAcceleratorTextForId(IDM_TOOLS_OPTIONS))
			.SetSelectedCallback([this] { SendCommandToBrowser(IDM_TOOLS_OPTIONS); }));
}

void ToolsMenu::SendCommandToBrowser(WORD id)
{
	SendMessage(m_browser->GetHWND(), WM_COMMAND, MAKEWPARAM(id, 0), 0);
}
