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

namespace
{

const wchar_t SETTING_ID[] = L"id";
const wchar_t SETTING_STYLE[] = L"Style";
const wchar_t SETTING_LENGTH[] = L"Length";

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

void SaveRebarBandInfo(HKEY key, const RebarBandStorageInfo &bandInfo)
{
	RegistrySettings::SaveDword(key, SETTING_ID, bandInfo.id);
	RegistrySettings::SaveDword(key, SETTING_STYLE, bandInfo.style);
	RegistrySettings::SaveDword(key, SETTING_LENGTH, bandInfo.length);
}

}

namespace MainRebarRegistryStorage
{

std::vector<RebarBandStorageInfo> Load(HKEY mainRebarKey)
{
	return RegistrySettings::ReadItemList<RebarBandStorageInfo>(mainRebarKey, LoadRebarBandInfo);
}

void Save(HKEY mainRebarKey, const std::vector<RebarBandStorageInfo> &rebarStorageInfo)
{
	RegistrySettings::SaveItemList<RebarBandStorageInfo>(mainRebarKey, rebarStorageInfo,
		SaveRebarBandInfo);
}

}
