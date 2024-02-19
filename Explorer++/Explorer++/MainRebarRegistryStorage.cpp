// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainRebarRegistryStorage.h"
#include "MainRebarStorage.h"
#include "../Helper/RegistrySettings.h"
#include <wil/registry.h>
#include <wil/resource.h>
#include <optional>

namespace MainRebarRegistryStorage
{

namespace
{

const WCHAR MAIN_REBAR_KEY_PATH[] = L"Toolbars";

const WCHAR SETTING_ID[] = L"id";
const WCHAR SETTING_STYLE[] = L"Style";
const WCHAR SETTING_LENGTH[] = L"Length";

std::optional<RebarBandStorageInfo> LoadRebarBandInfo(HKEY key)
{
	UINT id;
	LSTATUS result = RegistrySettings::Read32BitValueFromRegistry(key, SETTING_ID, id);

	if (result != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	UINT style;
	result = RegistrySettings::Read32BitValueFromRegistry(key, SETTING_STYLE, style);

	if (result != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	UINT length;
	result = RegistrySettings::Read32BitValueFromRegistry(key, SETTING_LENGTH, length);

	if (result != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	RebarBandStorageInfo bandInfo;
	bandInfo.id = id;
	bandInfo.style = style;
	bandInfo.length = length;
	return bandInfo;
}

std::vector<RebarBandStorageInfo> LoadFromKey(HKEY parentKey)
{
	std::vector<RebarBandStorageInfo> rebarStorageInfo;
	wil::unique_hkey childKey;
	size_t index = 0;

	while (SUCCEEDED(
		wil::reg::open_unique_key_nothrow(parentKey, std::to_wstring(index).c_str(), childKey)))
	{
		auto bandInfo = LoadRebarBandInfo(childKey.get());

		if (bandInfo)
		{
			rebarStorageInfo.push_back(*bandInfo);
		}

		index++;
	}

	return rebarStorageInfo;
}

void SaveRebarBandInfo(HKEY key, const RebarBandStorageInfo &bandInfo)
{
	RegistrySettings::SaveDword(key, SETTING_ID, bandInfo.id);
	RegistrySettings::SaveDword(key, SETTING_STYLE, bandInfo.style);
	RegistrySettings::SaveDword(key, SETTING_LENGTH, bandInfo.length);
}

void SaveToKey(HKEY parentKey, const std::vector<RebarBandStorageInfo> &rebarStorageInfo)
{
	size_t index = 0;

	for (const auto &bandInfo : rebarStorageInfo)
	{
		wil::unique_hkey childKey;
		HRESULT hr = wil::reg::create_unique_key_nothrow(parentKey, std::to_wstring(index).c_str(),
			childKey, wil::reg::key_access::readwrite);

		if (SUCCEEDED(hr))
		{
			SaveRebarBandInfo(childKey.get(), bandInfo);

			index++;
		}
	}
}

}

std::vector<RebarBandStorageInfo> Load(HKEY mainKey)
{
	wil::unique_hkey mainRebarKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(mainKey, MAIN_REBAR_KEY_PATH, mainRebarKey);

	if (FAILED(hr))
	{
		return {};
	}

	return LoadFromKey(mainRebarKey.get());
}

void Save(HKEY mainKey, const std::vector<RebarBandStorageInfo> &rebarStorageInfo)
{
	SHDeleteKey(mainKey, MAIN_REBAR_KEY_PATH);

	wil::unique_hkey mainRebarKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(mainKey, MAIN_REBAR_KEY_PATH, mainRebarKey,
		wil::reg::key_access::readwrite);

	if (FAILED(hr))
	{
		return;
	}

	SaveToKey(mainRebarKey.get(), rebarStorageInfo);
}

}
