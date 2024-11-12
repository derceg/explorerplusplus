// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WindowRegistryStorage.h"
#include "Storage.h"
#include "WindowStorage.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/WindowHelper.h"
#include <wil/registry.h>
#include <optional>

namespace
{

namespace V2
{

const wchar_t WINDOWS_KEY_PATH[] = L"Windows";

const wchar_t SETTING_X[] = L"X";
const wchar_t SETTING_Y[] = L"Y";
const wchar_t SETTING_WIDTH[] = L"Width";
const wchar_t SETTING_HEIGHT[] = L"Height";
const wchar_t SETTING_SHOW_STATE[] = L"ShowState";

std::optional<WindowStorageData> LoadWindow(HKEY key)
{
	int x;
	auto res = RegistrySettings::Read32BitValueFromRegistry(key, SETTING_X, x);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	int y;
	res = RegistrySettings::Read32BitValueFromRegistry(key, SETTING_Y, y);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	int width;
	res = RegistrySettings::Read32BitValueFromRegistry(key, SETTING_WIDTH, width);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	int height;
	res = RegistrySettings::Read32BitValueFromRegistry(key, SETTING_HEIGHT, height);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	WindowShowState showState = WindowShowState::Normal;
	RegistrySettings::ReadBetterEnumValue(key, SETTING_SHOW_STATE, showState);

	return WindowStorageData({ x, y, x + width, y + height }, showState);
}

std::vector<WindowStorageData> Load(HKEY windowsKey)
{
	std::vector<WindowStorageData> windows;
	wil::unique_hkey childKey;
	int index = 0;

	while (SUCCEEDED(
		wil::reg::open_unique_key_nothrow(windowsKey, std::to_wstring(index).c_str(), childKey)))
	{
		auto window = LoadWindow(childKey.get());

		if (window)
		{
			windows.push_back(*window);
		}

		index++;
	}

	return windows;
}

void SaveWindow(HKEY key, const WindowStorageData &window)
{
	RegistrySettings::SaveDword(key, SETTING_X, window.bounds.left);
	RegistrySettings::SaveDword(key, SETTING_Y, window.bounds.top);
	RegistrySettings::SaveDword(key, SETTING_WIDTH, GetRectWidth(&window.bounds));
	RegistrySettings::SaveDword(key, SETTING_HEIGHT, GetRectHeight(&window.bounds));
	RegistrySettings::SaveDword(key, SETTING_SHOW_STATE, window.showState);
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

namespace V1
{

const wchar_t SETTING_POSITION[] = L"Position";

std::optional<WindowStorageData> Load(HKEY settingsKey)
{
	WINDOWPLACEMENT placement;
	auto res = RegistrySettings::ReadBinaryValue(settingsKey, SETTING_POSITION, &placement,
		sizeof(placement));

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	return WindowStorageData(placement.rcNormalPosition,
		NativeShowStateToShowState(placement.showCmd));
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
		return V2::Load(windowsKey.get());
	}

	// Previously, the details for the main window were stored under the settings key.
	wil::unique_hkey settingsKey;
	hr = wil::reg::open_unique_key_nothrow(applicationKey, Storage::REGISTRY_SETTINGS_KEY_NAME,
		settingsKey, wil::reg::key_access::read);

	if (SUCCEEDED(hr))
	{
		// Previously, only a single window was stored.
		auto window = V1::Load(settingsKey.get());

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
