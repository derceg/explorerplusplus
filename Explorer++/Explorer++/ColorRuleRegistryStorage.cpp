// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColorRule.h"
#include "ColorRuleModel.h"
#include "../Helper/RegistrySettings.h"

namespace ColorRuleRegistryStorage
{

namespace
{

const wchar_t COLOR_RULES_KEY_PATH[] = L"ColorRules";

const wchar_t SETTING_DESCRIPTION[] = L"Description";
const wchar_t SETTING_FILENAME_PATTERN[] = L"FilenamePattern";
const wchar_t SETTING_CASE_INSENSITIVE[] = L"CaseInsensitive";
const wchar_t SETTING_ATTRIBUTES[] = L"Attributes";
const wchar_t SETTING_COLOR[] = L"Color";

std::unique_ptr<ColorRule> LoadColorRule(HKEY key)
{
	std::wstring description;
	LSTATUS result = RegistrySettings::ReadString(key, SETTING_DESCRIPTION, description);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	std::wstring filenamePattern;
	result = RegistrySettings::ReadString(key, SETTING_FILENAME_PATTERN, filenamePattern);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	bool caseInsensitive;
	result = RegistrySettings::Read32BitValueFromRegistry(key, SETTING_CASE_INSENSITIVE,
		caseInsensitive);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	DWORD attributes;
	result = RegistrySettings::ReadDword(key, SETTING_ATTRIBUTES, attributes);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	COLORREF color;
	result = RegistrySettings::ReadBinaryValue(key, SETTING_COLOR, &color, sizeof(color));

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	return std::make_unique<ColorRule>(description, filenamePattern, caseInsensitive, attributes,
		color);
}

void LoadFromKey(HKEY parentKey, ColorRuleModel *model)
{
	wil::unique_hkey childKey;
	size_t index = 0;

	while (RegOpenKeyEx(parentKey, std::to_wstring(index).c_str(), 0, KEY_READ, &childKey)
		== ERROR_SUCCESS)
	{
		auto colorRule = LoadColorRule(childKey.get());

		if (colorRule)
		{
			model->AddItem(std::move(colorRule));
		}

		index++;
	}
}

void SaveColorRule(HKEY key, const ColorRule *colorRule)
{
	RegistrySettings::SaveString(key, SETTING_DESCRIPTION, colorRule->GetDescription());
	RegistrySettings::SaveString(key, SETTING_FILENAME_PATTERN, colorRule->GetFilterPattern());
	RegistrySettings::SaveDword(key, SETTING_CASE_INSENSITIVE,
		colorRule->GetFilterPatternCaseInsensitive());
	RegistrySettings::SaveDword(key, SETTING_ATTRIBUTES, colorRule->GetFilterAttributes());

	COLORREF color = colorRule->GetColor();
	RegistrySettings::SaveBinaryValue(key, SETTING_COLOR, reinterpret_cast<const BYTE *>(&color),
		sizeof(color));
}

void SaveToKey(HKEY parentKey, const ColorRuleModel *model)
{
	size_t index = 0;

	for (const auto &colorRule : model->GetItems())
	{
		wil::unique_hkey childKey;
		LSTATUS res = RegCreateKeyEx(parentKey, std::to_wstring(index).c_str(), 0, nullptr,
			REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &childKey, nullptr);

		if (res == ERROR_SUCCESS)
		{
			SaveColorRule(childKey.get(), colorRule.get());

			index++;
		}
	}
}

}

void Load(HKEY applicationKey, ColorRuleModel *model)
{
	wil::unique_hkey colorRulesKey;
	LSTATUS res = RegOpenKeyEx(applicationKey, COLOR_RULES_KEY_PATH, 0, KEY_READ, &colorRulesKey);

	if (res == ERROR_SUCCESS)
	{
		model->RemoveAllItems();

		LoadFromKey(colorRulesKey.get(), model);
	}
}

void Save(HKEY applicationKey, const ColorRuleModel *model)
{
	wil::unique_hkey colorRulesKey;
	LSTATUS res = RegCreateKeyEx(applicationKey, COLOR_RULES_KEY_PATH, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &colorRulesKey, nullptr);

	if (res == ERROR_SUCCESS)
	{
		SaveToKey(colorRulesKey.get(), model);
	}
}

}
