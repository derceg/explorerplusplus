// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabRegistryStorage.h"
#include "ColumnRegistryStorage.h"
#include "ShellBrowser/FolderSettings.h"
#include "TabStorage.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"
#include <wil/registry.h>
#include <optional>

namespace TabRegistryStorage
{

namespace
{

// Required values
const wchar_t SETTING_DIRECTORY_PIDL[] = L"Directory";

// Folder settings
const wchar_t SETTING_VIEW_MODE[] = L"ViewMode";
const wchar_t SETTING_SORT_MODE[] = L"SortMode";
const wchar_t SETTING_SORT_ASCENDING[] = L"SortAscending";
const wchar_t SETTING_GROUP_MODE[] = L"GroupMode";
const wchar_t SETTING_GROUP_SORT_DIRECTION[] = L"GroupSortDirection";
const wchar_t SETTING_SHOW_IN_GROUPS[] = L"ShowInGroups";
const wchar_t SETTING_APPLY_FILTER[] = L"ApplyFilter";
const wchar_t SETTING_FILTER_CASE_SENSITIVE[] = L"FilterCaseSensitive";
const wchar_t SETTING_SHOW_HIDDEN[] = L"ShowHidden";
const wchar_t SETTING_AUTO_ARRANGE[] = L"AutoArrange";
const wchar_t SETTING_FILTER[] = L"Filter";

// Columns
const wchar_t SETTING_COLUMNS[] = L"Columns";

// Tab settings
const wchar_t SETTING_TAB_LOCKED[] = L"Locked";
const wchar_t SETTING_TAB_ADDRESS_LOCKED[] = L"AddressLocked";
const wchar_t SETTING_TAB_CUSTOM_NAME[] = L"CustomName";

void LoadBooleanSortDirection(HKEY key, const std::wstring &valueName, SortDirection &output)
{
	DWORD sortAscending;
	auto res = RegistrySettings::ReadDword(key, valueName, sortAscending);

	if (res != ERROR_SUCCESS)
	{
		return;
	}

	output = sortAscending ? SortDirection::Ascending : SortDirection::Descending;
}

FolderSettings LoadFolderSettings(HKEY key)
{
	FolderSettings folderSettings;

	RegistrySettings::ReadBetterEnumValue(key, SETTING_VIEW_MODE, folderSettings.viewMode);
	RegistrySettings::ReadBetterEnumValue(key, SETTING_SORT_MODE, folderSettings.sortMode);

	// Previously, the group mode and sort mode were always the same. Therefore, the group mode is
	// set here to preserve the behavior. This will only have an effect when updating from a version
	// that didn't save a separate group mode. If a group mode has been saved, it will be loaded
	// below and the value loaded here will be overwritten.
	folderSettings.groupMode = folderSettings.sortMode;

	LoadBooleanSortDirection(key, SETTING_SORT_ASCENDING, folderSettings.sortDirection);

	// As with the group mode/sort mode, the group sort direction and standard sort direction were
	// always the same in previous versions.
	folderSettings.groupSortDirection = folderSettings.sortDirection;

	RegistrySettings::ReadBetterEnumValue(key, SETTING_GROUP_MODE, folderSettings.groupMode);
	RegistrySettings::ReadBetterEnumValue(key, SETTING_GROUP_SORT_DIRECTION,
		folderSettings.groupSortDirection);
	RegistrySettings::Read32BitValueFromRegistry(key, SETTING_SHOW_IN_GROUPS,
		folderSettings.showInGroups);
	RegistrySettings::Read32BitValueFromRegistry(key, SETTING_APPLY_FILTER,
		folderSettings.filterEnabled);
	RegistrySettings::Read32BitValueFromRegistry(key, SETTING_FILTER_CASE_SENSITIVE,
		folderSettings.filterCaseSensitive);
	RegistrySettings::Read32BitValueFromRegistry(key, SETTING_SHOW_HIDDEN,
		folderSettings.showHidden);
	RegistrySettings::Read32BitValueFromRegistry(key, SETTING_AUTO_ARRANGE,
		folderSettings.autoArrangeEnabled);
	RegistrySettings::ReadString(key, SETTING_FILTER, folderSettings.filter);

	return folderSettings;
}

FolderColumns LoadColumns(HKEY key)
{
	FolderColumns columns;
	wil::unique_hkey columnsKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(key, SETTING_COLUMNS, columnsKey,
		wil::reg::key_access::read);

	if (SUCCEEDED(hr))
	{
		ColumnRegistryStorage::LoadAllColumnSets(columnsKey.get(), columns);
	}

	return columns;
}

TabSettings LoadTabSettings(HKEY key)
{
	TabSettings tabSettings;

	DWORD locked = 0;
	RegistrySettings::ReadDword(key, SETTING_TAB_LOCKED, locked);

	DWORD addressLocked = 0;
	RegistrySettings::ReadDword(key, SETTING_TAB_ADDRESS_LOCKED, addressLocked);

	if (addressLocked)
	{
		tabSettings.lockState = Tab::LockState::AddressLocked;
	}
	else if (locked)
	{
		tabSettings.lockState = Tab::LockState::Locked;
	}

	std::wstring customName;
	RegistrySettings::ReadString(key, SETTING_TAB_CUSTOM_NAME, customName);

	if (!customName.empty())
	{
		tabSettings.name = customName;
	}

	return tabSettings;
}

std::optional<TabStorageData> LoadTabInfo(HKEY key)
{
	PidlAbsolute pidl;
	auto res = RegistrySettings::ReadPidl(key, SETTING_DIRECTORY_PIDL, pidl);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	TabStorageData tabStorageData;
	tabStorageData.pidl = pidl;
	tabStorageData.folderSettings = LoadFolderSettings(key);
	tabStorageData.columns = LoadColumns(key);
	tabStorageData.tabSettings = LoadTabSettings(key);
	return tabStorageData;
}

void SaveFolderSettings(HKEY key, const FolderSettings &folderSettings)
{
	RegistrySettings::SaveDword(key, SETTING_VIEW_MODE, folderSettings.viewMode);
	RegistrySettings::SaveDword(key, SETTING_SORT_MODE, folderSettings.sortMode);

	// For backwards compatibility, the value saved here is a bool.
	RegistrySettings::SaveDword(key, SETTING_SORT_ASCENDING,
		folderSettings.sortDirection == +SortDirection::Ascending);

	RegistrySettings::SaveDword(key, SETTING_GROUP_MODE, folderSettings.groupMode);
	RegistrySettings::SaveDword(key, SETTING_GROUP_SORT_DIRECTION,
		folderSettings.groupSortDirection);
	RegistrySettings::SaveDword(key, SETTING_SHOW_IN_GROUPS, folderSettings.showInGroups);
	RegistrySettings::SaveDword(key, SETTING_APPLY_FILTER, folderSettings.filterEnabled);
	RegistrySettings::SaveDword(key, SETTING_FILTER_CASE_SENSITIVE,
		folderSettings.filterCaseSensitive);
	RegistrySettings::SaveDword(key, SETTING_SHOW_HIDDEN, folderSettings.showHidden);
	RegistrySettings::SaveDword(key, SETTING_AUTO_ARRANGE, folderSettings.autoArrangeEnabled);
	RegistrySettings::SaveString(key, SETTING_FILTER, folderSettings.filter);
}

void SaveColumns(HKEY key, const FolderColumns &columns)
{
	wil::unique_hkey columnsKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(key, SETTING_COLUMNS, columnsKey,
		wil::reg::key_access::readwrite);

	if (FAILED(hr))
	{
		return;
	}

	ColumnRegistryStorage::SaveAllColumnSets(columnsKey.get(), columns);
}

void SaveTabSettings(HKEY key, const TabSettings &tabSettings)
{
	RegistrySettings::SaveDword(key, SETTING_TAB_LOCKED,
		tabSettings.lockState == Tab::LockState::Locked);
	RegistrySettings::SaveDword(key, SETTING_TAB_ADDRESS_LOCKED,
		tabSettings.lockState == Tab::LockState::AddressLocked);

	std::wstring customName = tabSettings.name ? *tabSettings.name : L"";
	RegistrySettings::SaveString(key, SETTING_TAB_CUSTOM_NAME, customName);
}

void SaveTabInfo(HKEY key, const TabStorageData &tab)
{
	auto res = RegistrySettings::SavePidl(key, SETTING_DIRECTORY_PIDL, tab.pidl.Raw());

	if (res != ERROR_SUCCESS)
	{
		return;
	}

	SaveFolderSettings(key, tab.folderSettings);
	SaveColumns(key, tab.columns);
	SaveTabSettings(key, tab.tabSettings);
}

}

std::vector<TabStorageData> Load(HKEY tabsKey)
{
	return RegistrySettings::ReadItemList<TabStorageData>(tabsKey, LoadTabInfo);
}

void Save(HKEY tabsKey, const std::vector<TabStorageData> &tabs)
{
	RegistrySettings::SaveItemList<TabStorageData>(tabsKey, tabs, SaveTabInfo);
}

}
