// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RebarRegistryStorage.h"
#include "RebarBandStorageData.h"
#include "../Helper/RegistrySettings.h"
#include <optional>

namespace
{

const wchar_t SETTING_ID[] = L"id";
const wchar_t SETTING_STYLE[] = L"Style";
const wchar_t SETTING_LENGTH[] = L"Length";

std::optional<RebarBandStorageData> LoadRebarBandInfo(HKEY key)
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

	RebarBandStorageData bandInfo;
	bandInfo.id = id;
	bandInfo.style = style;
	bandInfo.length = length;
	return bandInfo;
}

void SaveRebarBandInfo(HKEY key, const RebarBandStorageData &bandInfo)
{
	RegistrySettings::SaveDword(key, SETTING_ID, bandInfo.id);
	RegistrySettings::SaveDword(key, SETTING_STYLE, bandInfo.style);
	RegistrySettings::SaveDword(key, SETTING_LENGTH, bandInfo.length);
}

}

namespace RebarRegistryStorage
{

std::vector<RebarBandStorageData> Load(HKEY rebarKey)
{
	return RegistrySettings::ReadItemList<RebarBandStorageData>(rebarKey, LoadRebarBandInfo);
}

void Save(HKEY rebarKey, const std::vector<RebarBandStorageData> &rebarStorageInfo)
{
	RegistrySettings::SaveItemList<RebarBandStorageData>(rebarKey, rebarStorageInfo,
		SaveRebarBandInfo);
}

}
