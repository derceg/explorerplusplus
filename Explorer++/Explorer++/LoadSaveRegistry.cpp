// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LoadSaveRegistry.h"
// clang-format off
#include "Explorer++.h"
// clang-format on
#include "App.h"
#include "ApplicationToolbarRegistryStorage.h"
#include "Bookmarks/BookmarkRegistryStorage.h"
#include "ColorRuleRegistryStorage.h"
#include "DefaultColumnRegistryStorage.h"
#include "DialogHelper.h"
#include "Storage.h"
#include <wil/registry.h>

LoadSaveRegistry::LoadSaveRegistry(App *app, Explorerplusplus *pContainer) :
	m_app(app),
	m_pContainer(pContainer)
{
}

void LoadSaveRegistry::LoadGenericSettings()
{
	wil::unique_hkey mainKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(HKEY_CURRENT_USER,
		Storage::REGISTRY_APPLICATION_KEY_PATH, mainKey, wil::reg::key_access::read);

	if (SUCCEEDED(hr))
	{
		m_pContainer->LoadGenericSettingsFromRegistry(mainKey.get());
	}
}

void LoadSaveRegistry::LoadPreviousTabs()
{
	m_pContainer->LoadTabSettingsFromRegistry();
}

void LoadSaveRegistry::LoadMainRebarInformation()
{
	wil::unique_hkey mainKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(HKEY_CURRENT_USER,
		Storage::REGISTRY_APPLICATION_KEY_PATH, mainKey, wil::reg::key_access::read);

	if (SUCCEEDED(hr))
	{
		m_pContainer->LoadMainRebarInformationFromRegistry(mainKey.get());
	}
}

void LoadSaveRegistry::SaveGenericSettings()
{
	wil::unique_hkey mainKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(HKEY_CURRENT_USER,
		Storage::REGISTRY_APPLICATION_KEY_PATH, mainKey, wil::reg::key_access::readwrite);

	if (SUCCEEDED(hr))
	{
		m_pContainer->SaveGenericSettingsToRegistry(mainKey.get());
	}
}

void LoadSaveRegistry::SaveBookmarks()
{
	wil::unique_hkey mainKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(HKEY_CURRENT_USER,
		Storage::REGISTRY_APPLICATION_KEY_PATH, mainKey, wil::reg::key_access::readwrite);

	if (SUCCEEDED(hr))
	{
		BookmarkRegistryStorage::Save(mainKey.get(), m_app->GetBookmarkTree());
	}
}

void LoadSaveRegistry::SaveTabs()
{
	m_pContainer->SaveTabSettingsToRegistry();
}

void LoadSaveRegistry::SaveDefaultColumns()
{
	wil::unique_hkey mainKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(HKEY_CURRENT_USER,
		Storage::REGISTRY_APPLICATION_KEY_PATH, mainKey, wil::reg::key_access::readwrite);

	if (SUCCEEDED(hr))
	{
		DefaultColumnRegistryStorage::Save(mainKey.get(),
			m_app->GetConfig()->globalFolderSettings.folderColumns);
	}
}

void LoadSaveRegistry::SaveApplicationToolbar()
{
	wil::unique_hkey mainKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(HKEY_CURRENT_USER,
		Storage::REGISTRY_APPLICATION_KEY_PATH, mainKey, wil::reg::key_access::readwrite);

	if (SUCCEEDED(hr))
	{
		Applications::ApplicationToolbarRegistryStorage::Save(mainKey.get(),
			m_app->GetApplicationModel());
	}
}

void LoadSaveRegistry::SaveMainRebarInformation()
{
	wil::unique_hkey mainKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(HKEY_CURRENT_USER,
		Storage::REGISTRY_APPLICATION_KEY_PATH, mainKey, wil::reg::key_access::readwrite);

	if (SUCCEEDED(hr))
	{
		m_pContainer->SaveMainRebarInformationToRegistry(mainKey.get());
	}
}

void LoadSaveRegistry::SaveColorRules()
{
	wil::unique_hkey mainKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(HKEY_CURRENT_USER,
		Storage::REGISTRY_APPLICATION_KEY_PATH, mainKey, wil::reg::key_access::readwrite);

	if (SUCCEEDED(hr))
	{
		ColorRuleRegistryStorage::Save(mainKey.get(), m_app->GetColorRuleModel());
	}
}

void LoadSaveRegistry::SaveDialogStates()
{
	wil::unique_hkey mainKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(HKEY_CURRENT_USER,
		Storage::REGISTRY_APPLICATION_KEY_PATH, mainKey, wil::reg::key_access::readwrite);

	if (SUCCEEDED(hr))
	{
		DialogHelper::SaveDialogStatesToRegistry(mainKey.get());
	}
}
