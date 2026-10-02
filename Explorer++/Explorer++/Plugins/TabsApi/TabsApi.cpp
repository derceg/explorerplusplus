// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Plugins/TabsApi/TabsApi.h"
#include "BrowserList.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "Plugins/TabsApi/TabProperties.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/SortModes.h"
#include "TabContainer.h"
#include "TabList.h"
#include <magic_enum/magic_enum.hpp>
#include <sol/sol.hpp>

Plugins::TabsApi::FolderSettings::FolderSettings(const ShellBrowserImpl &shellBrowser)
{
	sortMode = shellBrowser.GetSortMode();
	groupMode = shellBrowser.GetGroupMode();
	viewMode = shellBrowser.GetViewMode();
	autoArrange = shellBrowser.IsAutoArrangeEnabled();
	sortDirection = shellBrowser.GetSortDirection();
	groupSortDirection = shellBrowser.GetGroupSortDirection();
	showInGroups = shellBrowser.GetShowInGroups();
	showHidden = shellBrowser.GetShowHidden();
}

std::wstring Plugins::TabsApi::FolderSettings::ToString()
{
	// clang-format off
	return _T("sortMode = ") + utf8StrToWstr(magic_enum::enum_name(sortMode))
		+ _T(", groupMode = ") + utf8StrToWstr(magic_enum::enum_name(groupMode))
		+ _T(", viewMode = ") + utf8StrToWstr(magic_enum::enum_name(viewMode))
		+ _T(", autoArrange = ") + std::to_wstring(autoArrange)
		+ _T(", sortDirection = ") + utf8StrToWstr(magic_enum::enum_name(sortDirection))
		+ _T(", groupSortDirection = ") + utf8StrToWstr(magic_enum::enum_name(groupSortDirection))
		+ _T(", showInGroups = ") + std::to_wstring(showInGroups)
		+ _T(", showHidden = ") + std::to_wstring(showHidden);
	// clang-format on
}

Plugins::TabsApi::Tab::Tab(const ::Tab &tabInternal) :
	folderSettings(*tabInternal.GetShellBrowserImpl())
{
	id = tabInternal.GetId();
	location = tabInternal.GetShellBrowser()->GetDirectoryPath();
	name = tabInternal.GetName();
	locked = (tabInternal.GetLockState() == ::Tab::LockState::Locked);
	addressLocked = (tabInternal.GetLockState() == ::Tab::LockState::AddressLocked);
}

std::wstring Plugins::TabsApi::Tab::ToString()
{
	// clang-format off
	return _T("id = ") + std::to_wstring(id)
		+ _T(", location = ") + location
		+ _T(", name = ") + name
		+ _T(", locked = ") + std::to_wstring(locked)
		+ _T(", addressLocked = ") + std::to_wstring(addressLocked)
		+ _T(", folderSettings = {") + folderSettings.ToString() + _T("}");
	// clang-format on
}

Plugins::TabsApi::TabsApi(BrowserList *browserList, TabList *tabList, const Config *config) :
	m_browserList(browserList),
	m_tabList(tabList),
	m_config(config)
{
}

std::vector<Plugins::TabsApi::Tab> Plugins::TabsApi::GetAll()
{
	std::vector<Tab> tabs;

	for (auto *tabInternal : m_tabList->GetAll())
	{
		Tab tab(*tabInternal);
		tabs.push_back(tab);
	}

	return tabs;
}

std::optional<Plugins::TabsApi::Tab> Plugins::TabsApi::Get(int tabId)
{
	auto tabInternal = m_tabList->MaybeGetById(tabId);

	if (!tabInternal)
	{
		return std::nullopt;
	}

	Tab tab(*tabInternal);

	return tab;
}

int Plugins::TabsApi::Create(sol::table createProperties)
{
	auto *targetBrowser = m_browserList->GetLastActive();

	if (!targetBrowser)
	{
		return -1;
	}

	sol::optional<std::wstring> location = createProperties[TabConstants::LOCATION];

	if (!location || location->empty())
	{
		return -1;
	}

	TabSettings tabSettings;
	ExtractTabPropertiesForCreation(createProperties, tabSettings, targetBrowser);

	unique_pidl_absolute pidlDirectory;
	HRESULT hr = ParseDisplayNameForNavigation(location->c_str(), pidlDirectory);

	if (FAILED(hr))
	{
		return -1;
	}

	::FolderSettings folderSettings = m_config->defaultFolderSettings;

	sol::optional<sol::table> folderSettingsTable = createProperties[TabConstants::FOLDER_SETTINGS];

	if (folderSettingsTable)
	{
		ExtractFolderSettingsForCreation(*folderSettingsTable, folderSettings);
	}

	auto navigateParams = NavigateParams::Normal(pidlDirectory.get());
	auto &newTab = targetBrowser->GetActiveTabContainer()->CreateNewTab(navigateParams, tabSettings,
		&folderSettings);

	return newTab.GetId();
}

void Plugins::TabsApi::ExtractTabPropertiesForCreation(sol::table createProperties,
	TabSettings &tabSettings, BrowserWindow *targetBrowser)
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
		int numTabs = targetBrowser->GetActiveTabContainer()->GetNumTabs();

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

	if (lockState)
	{
		auto value = magic_enum::enum_cast<::Tab::LockState>(*lockState);

		if (value)
		{
			tabSettings.lockState = *value;
		}
	}
}

void Plugins::TabsApi::ExtractFolderSettingsForCreation(sol::table folderSettingsTable,
	::FolderSettings &folderSettings)
{
	sol::optional<int> sortMode = folderSettingsTable[FolderSettingsConstants::SORT_MODE];

	if (sortMode)
	{
		auto value = magic_enum::enum_cast<SortMode>(*sortMode);

		if (value)
		{
			folderSettings.sortMode = *value;
		}
	}

	sol::optional<int> groupMode = folderSettingsTable[FolderSettingsConstants::GROUP_MODE];

	if (groupMode)
	{
		auto value = magic_enum::enum_cast<SortMode>(*groupMode);

		if (value)
		{
			folderSettings.groupMode = *value;
		}
	}

	sol::optional<int> viewMode = folderSettingsTable[FolderSettingsConstants::VIEW_MODE];

	if (viewMode)
	{
		auto value = magic_enum::enum_cast<ViewMode>(*viewMode);

		if (value)
		{
			folderSettings.viewMode = *value;
		}
	}

	sol::optional<bool> autoArrange = folderSettingsTable[FolderSettingsConstants::AUTO_ARRANGE];

	if (autoArrange)
	{
		folderSettings.autoArrangeEnabled = *autoArrange;
	}

	sol::optional<int> sortDirection = folderSettingsTable[FolderSettingsConstants::SORT_DIRECTION];

	if (sortDirection)
	{
		auto value = magic_enum::enum_cast<SortDirection>(*sortDirection);

		if (value)
		{
			folderSettings.sortDirection = *value;
		}
	}

	sol::optional<int> groupSortDirection =
		folderSettingsTable[FolderSettingsConstants::GROUP_SORT_DIRECTION];

	if (groupSortDirection)
	{
		auto value = magic_enum::enum_cast<SortDirection>(*groupSortDirection);

		if (value)
		{
			folderSettings.groupSortDirection = *value;
		}
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

void Plugins::TabsApi::Update(int tabId, sol::table properties)
{
	auto tabInternal = m_tabList->MaybeGetById(tabId);

	if (!tabInternal)
	{
		return;
	}

	sol::optional<std::wstring> location = properties[TabConstants::LOCATION];

	if (location && !location->empty())
	{
		tabInternal->GetShellBrowser()->GetNavigationController()->Navigate(*location);
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

	if (lockState)
	{
		auto value = magic_enum::enum_cast<::Tab::LockState>(*lockState);

		if (value)
		{
			tabInternal->SetLockState(*value);
		}
	}

	sol::optional<bool> active = properties[TabConstants::ACTIVE];

	if (active && *active)
	{
		tabInternal->GetTabContainer()->SelectTab(*tabInternal);
	}
}

void Plugins::TabsApi::Refresh(int tabId)
{
	auto tabInternal = m_tabList->MaybeGetById(tabId);

	if (!tabInternal)
	{
		return;
	}

	tabInternal->GetShellBrowser()->GetNavigationController()->Refresh();
}

int Plugins::TabsApi::Move(int tabId, int newIndex)
{
	auto tabInternal = m_tabList->MaybeGetById(tabId);

	if (!tabInternal)
	{
		return -1;
	}

	auto *tabContainer = tabInternal->GetTabContainer();

	if (newIndex < 0)
	{
		newIndex = tabContainer->GetNumTabs();
	}

	return tabContainer->MoveTab(*tabInternal, newIndex);
}

bool Plugins::TabsApi::Close(int tabId)
{
	auto tabInternal = m_tabList->MaybeGetById(tabId);

	if (!tabInternal)
	{
		return false;
	}

	return tabInternal->GetTabContainer()->CloseTab(*tabInternal);
}
