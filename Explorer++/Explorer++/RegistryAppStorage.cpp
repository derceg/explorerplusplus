// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RegistryAppStorage.h"
#include "ApplicationToolbarRegistryStorage.h"
#include "Bookmarks/BookmarkRegistryStorage.h"
#include "ColorRuleRegistryStorage.h"
#include "ConfigRegistryStorage.h"
#include "DefaultColumnRegistryStorage.h"
#include "DialogStorageHelper.h"
#include "FrequentLocationsRegistryStorage.h"
#include "MainRebarStorage.h"
#include "TabStorage.h"
#include "WindowRegistryStorage.h"
#include "WindowStorage.h"

RegistryAppStorage::RegistryAppStorage(wil::unique_hkey applicationKey) :
	m_applicationKey(std::move(applicationKey))
{
}

void RegistryAppStorage::LoadConfig(Config &config)
{
	ConfigRegistryStorage::Load(m_applicationKey.get(), config);
}

std::vector<WindowStorageData> RegistryAppStorage::LoadWindows()
{
	return WindowRegistryStorage::Load(m_applicationKey.get());
}

void RegistryAppStorage::LoadBookmarks(BookmarkTree *bookmarkTree)
{
	BookmarkRegistryStorage::Load(m_applicationKey.get(), bookmarkTree);
}

void RegistryAppStorage::LoadColorRules(ColorRuleModel *model)
{
	ColorRuleRegistryStorage::Load(m_applicationKey.get(), model);
}

void RegistryAppStorage::LoadApplications(Applications::ApplicationModel *model)
{
	Applications::ApplicationToolbarRegistryStorage::Load(m_applicationKey.get(), model);
}

void RegistryAppStorage::LoadDialogStates()
{
	DialogStorageHelper::LoadDialogStatesFromRegistry(m_applicationKey.get());
}

void RegistryAppStorage::LoadDefaultColumns(FolderColumns &defaultColumns)
{
	DefaultColumnRegistryStorage::Load(m_applicationKey.get(), defaultColumns);
}

void RegistryAppStorage::LoadFrequentLocations(FrequentLocationsModel *frequentLocationsModel)
{
	FrequentLocationsRegistryStorage::Load(m_applicationKey.get(), frequentLocationsModel);
}

void RegistryAppStorage::SaveConfig(const Config &config)
{
	ConfigRegistryStorage::Save(m_applicationKey.get(), config);
}

void RegistryAppStorage::SaveWindows(const std::vector<WindowStorageData> &windows)
{
	WindowRegistryStorage::Save(m_applicationKey.get(), windows);
}

void RegistryAppStorage::SaveBookmarks(const BookmarkTree *bookmarkTree)
{
	BookmarkRegistryStorage::Save(m_applicationKey.get(), bookmarkTree);
}

void RegistryAppStorage::SaveColorRules(const ColorRuleModel *model)
{
	ColorRuleRegistryStorage::Save(m_applicationKey.get(), model);
}

void RegistryAppStorage::SaveApplications(const Applications::ApplicationModel *model)
{
	Applications::ApplicationToolbarRegistryStorage::Save(m_applicationKey.get(), model);
}

void RegistryAppStorage::SaveDialogStates()
{
	DialogStorageHelper::SaveDialogStatesToRegistry(m_applicationKey.get());
}

void RegistryAppStorage::SaveDefaultColumns(const FolderColumns &defaultColumns)
{
	DefaultColumnRegistryStorage::Save(m_applicationKey.get(), defaultColumns);
}

void RegistryAppStorage::SaveFrequentLocations(const FrequentLocationsModel *frequentLocationsModel)
{
	FrequentLocationsRegistryStorage::Save(m_applicationKey.get(), frequentLocationsModel);
}

void RegistryAppStorage::Commit()
{
}
