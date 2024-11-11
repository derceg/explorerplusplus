// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationToolbarRegistryStorage.h"
#include "Application.h"
#include "ApplicationModel.h"
#include "../Helper/RegistrySettings.h"

namespace Applications
{

namespace ApplicationToolbarRegistryStorage
{

namespace
{

const TCHAR APPLICATION_TOOLBAR_KEY_PATH[] = _T("ApplicationToolbar");

const TCHAR SETTING_NAME[] = _T("Name");
const TCHAR SETTING_COMMAND[] = _T("Command");
const TCHAR SETTING_SHOW_NAME_ON_TOOLBAR[] = _T("ShowNameOnToolbar");

std::unique_ptr<Application> LoadApplication(HKEY key)
{
	std::wstring name;
	LSTATUS result = RegistrySettings::ReadString(key, SETTING_NAME, name);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	std::wstring command;
	result = RegistrySettings::ReadString(key, SETTING_COMMAND, command);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	bool showNameOnToolbar;
	result = RegistrySettings::Read32BitValueFromRegistry(key, SETTING_SHOW_NAME_ON_TOOLBAR,
		showNameOnToolbar);

	if (result != ERROR_SUCCESS)
	{
		return nullptr;
	}

	return std::make_unique<Application>(name, command, showNameOnToolbar);
}

void LoadFromKey(HKEY parentKey, ApplicationModel *model)
{
	wil::unique_hkey childKey;
	size_t index = 0;

	while (RegOpenKeyEx(parentKey, std::to_wstring(index).c_str(), 0, KEY_READ, &childKey)
		== ERROR_SUCCESS)
	{
		auto application = LoadApplication(childKey.get());

		if (application)
		{
			model->AddItem(std::move(application));
		}

		index++;
	}
}

void SaveApplication(HKEY key, const Application *application)
{
	RegistrySettings::SaveString(key, SETTING_NAME, application->GetName());
	RegistrySettings::SaveString(key, SETTING_COMMAND, application->GetCommand());
	RegistrySettings::SaveDword(key, SETTING_SHOW_NAME_ON_TOOLBAR,
		application->GetShowNameOnToolbar());
}

void SaveToKey(HKEY parentKey, const ApplicationModel *model)
{
	size_t index = 0;

	for (const auto &application : model->GetItems())
	{
		wil::unique_hkey childKey;
		LSTATUS res = RegCreateKeyEx(parentKey, std::to_wstring(index).c_str(), 0, nullptr,
			REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &childKey, nullptr);

		if (res == ERROR_SUCCESS)
		{
			SaveApplication(childKey.get(), application.get());

			index++;
		}
	}
}

}

void Load(HKEY applicationKey, ApplicationModel *model)
{
	wil::unique_hkey applicationToolbarKey;
	LSTATUS res = RegOpenKeyEx(applicationKey, APPLICATION_TOOLBAR_KEY_PATH, 0, KEY_READ,
		&applicationToolbarKey);

	if (res == ERROR_SUCCESS)
	{
		LoadFromKey(applicationToolbarKey.get(), model);
	}
}

void Save(HKEY applicationKey, const ApplicationModel *model)
{
	wil::unique_hkey applicationToolbarKey;
	LSTATUS res = RegCreateKeyEx(applicationKey, APPLICATION_TOOLBAR_KEY_PATH, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &applicationToolbarKey, nullptr);

	if (res == ERROR_SUCCESS)
	{
		SaveToKey(applicationToolbarKey.get(), model);
	}
}

}

}
