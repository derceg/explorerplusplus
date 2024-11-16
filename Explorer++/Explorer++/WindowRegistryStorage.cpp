// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WindowRegistryStorage.h"
#include "Storage.h"
#include "TabRegistryStorage.h"
#include "TabStorage.h"
#include "WindowStorage.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/WindowHelper.h"
#include <wil/registry.h>
#include <optional>

namespace
{

namespace V1
{

const wchar_t SETTING_POSITION[] = L"Position";
const wchar_t SETTING_SELECTED_TAB[] = L"LastSelectedTab";

const wchar_t TABS_SUB_KEY_PATH[] = L"Tabs";

std::vector<TabStorageData> LoadTabs(HKEY applicationKey)
{
	wil::unique_hkey tabsKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(applicationKey, TABS_SUB_KEY_PATH, tabsKey,
		wil::reg::key_access::read);

	if (FAILED(hr))
	{
		return {};
	}

	return TabRegistryStorage::Load(tabsKey.get());
}

std::optional<WindowStorageData> Load(HKEY applicationKey, HKEY settingsKey)
{
	WINDOWPLACEMENT placement;
	auto res = RegistrySettings::ReadBinaryValue(settingsKey, SETTING_POSITION, &placement,
		sizeof(placement));

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	auto tabs = LoadTabs(applicationKey);

	int selectedTab = 0;
	RegistrySettings::Read32BitValueFromRegistry(settingsKey, SETTING_SELECTED_TAB, selectedTab);

	return WindowStorageData(placement.rcNormalPosition,
		NativeShowStateToShowState(placement.showCmd), tabs, selectedTab);
}

}

namespace V2
{

const wchar_t WINDOWS_KEY_PATH[] = L"Windows";

const wchar_t SETTING_X[] = L"X";
const wchar_t SETTING_Y[] = L"Y";
const wchar_t SETTING_WIDTH[] = L"Width";
const wchar_t SETTING_HEIGHT[] = L"Height";
const wchar_t SETTING_SHOW_STATE[] = L"ShowState";
const wchar_t SETTING_SELECTED_TAB[] = L"SelectedTab";

const wchar_t TABS_SUB_KEY_PATH[] = L"Tabs";

std::optional<WindowStorageData> LoadWindow(HKEY applicationKey, HKEY windowKey, bool fallback)
{
	int x;
	auto res = RegistrySettings::Read32BitValueFromRegistry(windowKey, SETTING_X, x);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	int y;
	res = RegistrySettings::Read32BitValueFromRegistry(windowKey, SETTING_Y, y);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	int width;
	res = RegistrySettings::Read32BitValueFromRegistry(windowKey, SETTING_WIDTH, width);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	int height;
	res = RegistrySettings::Read32BitValueFromRegistry(windowKey, SETTING_HEIGHT, height);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	WindowShowState showState = WindowShowState::Normal;
	RegistrySettings::ReadBetterEnumValue(windowKey, SETTING_SHOW_STATE, showState);

	std::vector<TabStorageData> tabs;

	if (wil::unique_hkey tabsKey; SUCCEEDED(wil::reg::open_unique_key_nothrow(windowKey,
			TABS_SUB_KEY_PATH, tabsKey, wil::reg::key_access::read)))
	{
		tabs = TabRegistryStorage::Load(tabsKey.get());
	}
	else if (fallback)
	{
		tabs = V1::LoadTabs(applicationKey);
	}

	int selectedTab = 0;
	res =
		RegistrySettings::Read32BitValueFromRegistry(windowKey, SETTING_SELECTED_TAB, selectedTab);

	if (res != ERROR_SUCCESS && fallback)
	{
		wil::unique_hkey settingsKey;
		HRESULT hr = wil::reg::open_unique_key_nothrow(applicationKey,
			Storage::REGISTRY_SETTINGS_KEY_NAME, settingsKey, wil::reg::key_access::read);

		if (SUCCEEDED(hr))
		{
			RegistrySettings::Read32BitValueFromRegistry(settingsKey.get(),
				V1::SETTING_SELECTED_TAB, selectedTab);
		}
	}

	return WindowStorageData({ x, y, x + width, y + height }, showState, tabs, selectedTab);
}

std::vector<WindowStorageData> Load(HKEY applicationKey, HKEY windowsKey)
{
	std::vector<WindowStorageData> windows;
	wil::unique_hkey childKey;
	int index = 0;
	bool fallback = true;

	while (SUCCEEDED(
		wil::reg::open_unique_key_nothrow(windowsKey, std::to_wstring(index).c_str(), childKey)))
	{
		auto window = LoadWindow(applicationKey, childKey.get(), fallback);

		if (window)
		{
			windows.push_back(*window);
		}

		// Originally, only a single window was saved and the details for that window were saved
		// under other top-level keys. So, for the first window that's loaded, it's possible to fall
		// back to the original keys, since the data may be saved there. Fallback shouldn't be used
		// for any additional windows, however.
		fallback = false;

		index++;
	}

	return windows;
}

void SaveWindow(HKEY windowKey, const WindowStorageData &window)
{
	RegistrySettings::SaveDword(windowKey, SETTING_X, window.bounds.left);
	RegistrySettings::SaveDword(windowKey, SETTING_Y, window.bounds.top);
	RegistrySettings::SaveDword(windowKey, SETTING_WIDTH, GetRectWidth(&window.bounds));
	RegistrySettings::SaveDword(windowKey, SETTING_HEIGHT, GetRectHeight(&window.bounds));
	RegistrySettings::SaveDword(windowKey, SETTING_SHOW_STATE, window.showState);
	RegistrySettings::SaveDword(windowKey, SETTING_SELECTED_TAB, window.selectedTab);

	wil::unique_hkey tabsKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(windowKey, TABS_SUB_KEY_PATH, tabsKey,
		wil::reg::key_access::readwrite);

	if (SUCCEEDED(hr))
	{
		TabRegistryStorage::Save(tabsKey.get(), window.tabs);
	}
}

void Save(HKEY windowsKey, const std::vector<WindowStorageData> &windows)
{
	size_t index = 0;

	for (const auto &window : windows)
	{
		wil::unique_hkey childKey;
		HRESULT hr = wil::reg::create_unique_key_nothrow(windowsKey, std::to_wstring(index).c_str(),
			childKey, wil::reg::key_access::readwrite);

		if (SUCCEEDED(hr))
		{
			SaveWindow(childKey.get(), window);

			index++;
		}
	}
}

}

}

namespace WindowRegistryStorage
{

std::vector<WindowStorageData> Load(HKEY applicationKey)
{
	wil::unique_hkey windowsKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(applicationKey, V2::WINDOWS_KEY_PATH, windowsKey,
		wil::reg::key_access::read);

	if (SUCCEEDED(hr))
	{
		return V2::Load(applicationKey, windowsKey.get());
	}

	// Previously, the details for the main window were stored under the settings key.
	wil::unique_hkey settingsKey;
	hr = wil::reg::open_unique_key_nothrow(applicationKey, Storage::REGISTRY_SETTINGS_KEY_NAME,
		settingsKey, wil::reg::key_access::read);

	if (SUCCEEDED(hr))
	{
		// Previously, only a single window was stored.
		auto window = V1::Load(applicationKey, settingsKey.get());

		if (window)
		{
			return { *window };
		}
	}

	return {};
}

void Save(HKEY applicationKey, const std::vector<WindowStorageData> &windows)
{
	wil::unique_hkey windowsKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(applicationKey, V2::WINDOWS_KEY_PATH,
		windowsKey, wil::reg::key_access::readwrite);

	if (SUCCEEDED(hr))
	{
		V2::Save(windowsKey.get(), windows);
	}
}

}
