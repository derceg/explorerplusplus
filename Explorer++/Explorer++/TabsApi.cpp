// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabsAPI.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/SortModes.h"
#include "TabProperties.h"
#include <boost/scope_exit.hpp>

#pragma warning(disable:4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

Plugins::TabsApi::TabsApi(TabContainer *tabContainer, TabInterface *tabInterface, Navigation *navigation) :
	m_tabContainer(tabContainer),
	m_tabInterface(tabInterface),
	m_navigation(navigation)
{

}

Plugins::TabsApi::~TabsApi()
{

}

std::vector<Plugins::TabsApi::Tab> Plugins::TabsApi::getAll()
{
	std::vector<Tab> tabs;

	for (auto &item : m_tabContainer->GetAllTabs())
	{
		Tab tab(item.second);
		tabs.push_back(tab);
	}

	return tabs;
}

boost::optional<Plugins::TabsApi::Tab> Plugins::TabsApi::get(int tabId)
{
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

	if (!tabInternal)
	{
		return boost::none;
	}

	Tab tab(*tabInternal);

	return tab;
}

int Plugins::TabsApi::create(sol::table createProperties)
{
	boost::optional<std::wstring> location = createProperties[TabConstants::LOCATION];

	if (!location || location->empty())
	{
		return -1;
	}

	TabSettings tabSettings;
	extractTabPropertiesForCreation(createProperties, tabSettings);

	LPITEMIDLIST pidlDirectory;
	HRESULT hr = GetIdlFromParsingName(location->c_str(), &pidlDirectory);

	if (FAILED(hr))
	{
		return -1;
	}

	BOOST_SCOPE_EXIT(pidlDirectory) {
		CoTaskMemFree(pidlDirectory);
	} BOOST_SCOPE_EXIT_END

	::FolderSettings folderSettings = m_tabContainer->GetDefaultFolderSettings(pidlDirectory);

	boost::optional<sol::table> folderSettingsTable = createProperties[TabConstants::FOLDER_SETTINGS];

	if (folderSettingsTable)
	{
		extractFolderSettingsForCreation(*folderSettingsTable, folderSettings);
	}

	int tabId;
	hr = m_tabContainer->CreateNewTab(pidlDirectory, tabSettings, &folderSettings, boost::none, &tabId);

	if (FAILED(hr))
	{
		/* TODO: Ideally, an error message would be available in case of
		failure. */
		return -1;
	}

	return tabId;
}

void Plugins::TabsApi::extractTabPropertiesForCreation(sol::table createProperties, TabSettings &tabSettings)
{
	boost::optional<std::wstring> name = createProperties[TabConstants::NAME];

	if (name && !name->empty())
	{
		tabSettings.name = name;
	}

	boost::optional<int> index = createProperties[TabConstants::INDEX];

	if (index)
	{
		int finalIndex = *index;
		int numTabs = m_tabContainer->GetNumTabs();

		if (finalIndex < 0)
		{
			finalIndex = 0;
		}
		else if (finalIndex > numTabs)
		{
			finalIndex = numTabs;
		}

		tabSettings.index = finalIndex;
	}

	boost::optional<bool> active = createProperties[TabConstants::ACTIVE];

	if (active)
	{
		tabSettings.selected = *active;
	}

	boost::optional<bool> locked = createProperties[TabConstants::LOCKED];

	if (locked)
	{
		tabSettings.locked = *locked;
	}

	boost::optional<bool> addressLocked = createProperties[TabConstants::ADDRESS_LOCKED];

	if (addressLocked)
	{
		tabSettings.addressLocked = *addressLocked;
	}
}

void Plugins::TabsApi::extractFolderSettingsForCreation(sol::table folderSettingsTable, ::FolderSettings &folderSettings)
{
	boost::optional<int> sortMode = folderSettingsTable[FolderSettingsConstants::SORT_MODE];

	if (sortMode && SortMode::_is_valid(*sortMode))
	{
		folderSettings.sortMode = SortMode::_from_integral(*sortMode);
	}
	
	boost::optional<int> viewMode = folderSettingsTable[FolderSettingsConstants::VIEW_MODE];

	if (viewMode && ViewMode::_is_valid(*viewMode))
	{
		folderSettings.viewMode = ViewMode::_from_integral(*viewMode);
	}

	boost::optional<bool> autoArrange = folderSettingsTable[FolderSettingsConstants::AUTO_ARRANGE];

	if (autoArrange)
	{
		folderSettings.autoArrange = *autoArrange;
	}

	boost::optional<bool> sortAscending = folderSettingsTable[FolderSettingsConstants::SORT_ASCENDING];

	if (sortAscending)
	{
		folderSettings.sortAscending = *sortAscending;
	}

	boost::optional<bool> showInGroups = folderSettingsTable[FolderSettingsConstants::SHOW_IN_GROUPS];

	if (showInGroups)
	{
		folderSettings.showInGroups = *showInGroups;
	}

	boost::optional<bool> showHidden = folderSettingsTable[FolderSettingsConstants::SHOW_HIDDEN];

	if (showHidden)
	{
		folderSettings.showHidden = *showHidden;
	}
}

void Plugins::TabsApi::update(int tabId, sol::table properties)
{
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

	if (!tabInternal)
	{
		return;
	}

	boost::optional<std::wstring> location = properties[TabConstants::LOCATION];

	if (location && !location->empty())
	{
		m_navigation->BrowseFolder(*tabInternal, location->c_str(), SBSP_ABSOLUTE);
	}

	boost::optional<std::wstring> name = properties[TabConstants::NAME];

	if (name)
	{
		if (name->empty())
		{
			tabInternal->ClearCustomName();
		}
		else
		{
			tabInternal->SetCustomName(*name);
		}
	}

	boost::optional<bool> locked = properties[TabConstants::LOCKED];

	if (locked)
	{
		tabInternal->SetLocked(*locked);
	}

	boost::optional<bool> addressLocked = properties[TabConstants::ADDRESS_LOCKED];

	if (addressLocked)
	{
		tabInternal->SetAddressLocked(*addressLocked);
	}

	boost::optional<bool> active = properties[TabConstants::ACTIVE];

	if (active && *active)
	{
		m_tabContainer->SelectTab(*tabInternal);
	}
}

void Plugins::TabsApi::refresh(int tabId)
{
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

	if (!tabInternal)
	{
		return;
	}

	m_tabInterface->RefreshTab(*tabInternal);
}

int Plugins::TabsApi::move(int tabId, int newIndex)
{
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

	if (!tabInternal)
	{
		return -1;
	}

	if (newIndex < 0)
	{
		newIndex = m_tabContainer->GetNumTabs();
	}

	return m_tabContainer->MoveTab(*tabInternal, newIndex);
}

bool Plugins::TabsApi::close(int tabId)
{
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

	if (!tabInternal)
	{
		return false;
	}

	return m_tabContainer->CloseTab(*tabInternal);
}