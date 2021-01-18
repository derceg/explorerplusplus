// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/TabsApi/TabsApi.h"
#include "Config.h"
#include "CoreInterface.h"
#include "Navigation.h"
#include "Plugins/TabsApi/TabProperties.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/SortModes.h"
#include "SolWrapper.h"
#include "TabContainer.h"

Plugins::TabsApi::FolderSettings::FolderSettings(const ShellBrowser &shellBrowser)
{
	sortMode = shellBrowser.GetSortMode();
	viewMode = shellBrowser.GetViewMode();
	sortAscending = shellBrowser.GetSortAscending();
	showInGroups = shellBrowser.GetShowInGroups();
	showHidden = shellBrowser.GetShowHidden();
	autoArrange = shellBrowser.GetAutoArrange();
}

std::wstring Plugins::TabsApi::FolderSettings::toString()
{
	return _T("sortMode = ") + utf8StrToWstr(sortMode._to_string())
		+ _T(", viewMode = ") + utf8StrToWstr(viewMode._to_string())
		+ _T(", sortAscending = ") + std::to_wstring(sortAscending)
		+ _T(", showInGroups = ") + std::to_wstring(showInGroups)
		+ _T(", showHidden = ") + std::to_wstring(showHidden)
		+ _T(", autoArrange = ") + std::to_wstring(autoArrange);
}

Plugins::TabsApi::Tab::Tab(const ::Tab &tabInternal) :
	folderSettings(*tabInternal.GetShellBrowser())
{
	id = tabInternal.GetId();
	location = tabInternal.GetShellBrowser()->GetDirectory();
	name = tabInternal.GetName();
	locked = (tabInternal.GetLockState() == ::Tab::LockState::Locked);
	addressLocked = (tabInternal.GetLockState() == ::Tab::LockState::AddressLocked);
}

std::wstring Plugins::TabsApi::Tab::toString()
{
	return _T("id = ") + std::to_wstring(id)
		+ _T(", location = ") + location
		+ _T(", name = ") + name
		+ _T(", locked = ") + std::to_wstring(locked)
		+ _T(", addressLocked = ") + std::to_wstring(addressLocked)
		+ _T(", folderSettings = {") + folderSettings.toString() + _T("}");
}

Plugins::TabsApi::TabsApi(IExplorerplusplus *expp, TabContainer *tabContainer) :
	m_expp(expp),
	m_tabContainer(tabContainer)
{

}

std::vector<Plugins::TabsApi::Tab> Plugins::TabsApi::getAll()
{
	std::vector<Tab> tabs;

	for (auto &item : m_tabContainer->GetAllTabs())
	{
		Tab tab(*item.second);
		tabs.push_back(tab);
	}

	return tabs;
}

std::optional<Plugins::TabsApi::Tab> Plugins::TabsApi::get(int tabId)
{
	auto tabInternal = m_tabContainer->GetTabOptional(tabId);

	if (!tabInternal)
	{
		return std::nullopt;
	}

	Tab tab(*tabInternal);

	return tab;
}

int Plugins::TabsApi::create(sol::table createProperties)
{
	sol::optional<std::wstring> location = createProperties[TabConstants::LOCATION];

	if (!location || location->empty())
	{
		return -1;
	}

	TabSettings tabSettings;
	extractTabPropertiesForCreation(createProperties, tabSettings);

	unique_pidl_absolute pidlDirectory;
	HRESULT hr = SHParseDisplayName(location->c_str(), nullptr, wil::out_param(pidlDirectory), 0, nullptr);

	if (FAILED(hr))
	{
		return -1;
	}

	::FolderSettings folderSettings = m_expp->GetConfig()->defaultFolderSettings;

	sol::optional<sol::table> folderSettingsTable = createProperties[TabConstants::FOLDER_SETTINGS];

	if (folderSettingsTable)
	{
		extractFolderSettingsForCreation(*folderSettingsTable, folderSettings);
	}

	int tabId = -1;
	m_tabContainer->CreateNewTab(pidlDirectory.get(), tabSettings, &folderSettings, std::nullopt, &tabId);

	return tabId;
}

void Plugins::TabsApi::extractTabPropertiesForCreation(sol::table createProperties, TabSettings &tabSettings)
{
	sol::optional<std::wstring> name = createProperties[TabConstants::NAME];

	if (name && !name->empty())
	{
		tabSettings.name = *name;
	}

	sol::optional<int> index = createProperties[TabConstants::INDEX];

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

	sol::optional<bool> active = createProperties[TabConstants::ACTIVE];

	if (active)
	{
		tabSettings.selected = *active;
	}

	sol::optional<int> lockState = createProperties[TabConstants::LOCK_STATE];

	// TODO: Verify that lockState has a valid value.
	if (lockState)
	{
		tabSettings.lockState = static_cast<::Tab::LockState>(*lockState);
	}
}

void Plugins::TabsApi::extractFolderSettingsForCreation(sol::table folderSettingsTable, ::FolderSettings &folderSettings)
{
	sol::optional<int> sortMode = folderSettingsTable[FolderSettingsConstants::SORT_MODE];

	if (sortMode && SortMode::_is_valid(*sortMode))
	{
		folderSettings.sortMode = SortMode::_from_integral(*sortMode);
	}
	
	sol::optional<int> viewMode = folderSettingsTable[FolderSettingsConstants::VIEW_MODE];

	if (viewMode && ViewMode::_is_valid(*viewMode))
	{
		folderSettings.viewMode = ViewMode::_from_integral(*viewMode);
	}

	sol::optional<bool> autoArrange = folderSettingsTable[FolderSettingsConstants::AUTO_ARRANGE];

	if (autoArrange)
	{
		folderSettings.autoArrange = *autoArrange;
	}

	sol::optional<bool> sortAscending = folderSettingsTable[FolderSettingsConstants::SORT_ASCENDING];

	if (sortAscending)
	{
		folderSettings.sortAscending = *sortAscending;
	}

	sol::optional<bool> showInGroups = folderSettingsTable[FolderSettingsConstants::SHOW_IN_GROUPS];

	if (showInGroups)
	{
		folderSettings.showInGroups = *showInGroups;
	}

	sol::optional<bool> showHidden = folderSettingsTable[FolderSettingsConstants::SHOW_HIDDEN];

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

	sol::optional<std::wstring> location = properties[TabConstants::LOCATION];

	if (location && !location->empty())
	{
		tabInternal->GetShellBrowser()->GetNavigationController()->BrowseFolder(*location);
	}

	sol::optional<std::wstring> name = properties[TabConstants::NAME];

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

	sol::optional<int> lockState = properties[TabConstants::LOCK_STATE];

	// TODO: Verify that lockState has a valid value.
	if (lockState)
	{
		tabInternal->SetLockState(static_cast<::Tab::LockState> (*lockState));
	}

	sol::optional<bool> active = properties[TabConstants::ACTIVE];

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

	tabInternal->GetShellBrowser()->GetNavigationController()->Refresh();
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