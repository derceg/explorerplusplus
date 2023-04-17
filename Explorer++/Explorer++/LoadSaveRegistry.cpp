// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LoadSaveRegistry.h"
// clang-format off
#include "Explorer++.h"
// clang-format on
#include "ApplicationModelFactory.h"
#include "ApplicationToolbarRegistryStorage.h"
#include "Bookmarks/BookmarkRegistryStorage.h"
#include "Bookmarks/BookmarkTreeFactory.h"
#include "ColorRuleModelFactory.h"
#include "ColorRuleRegistryStorage.h"
#include "DialogHelper.h"
#include "Explorer++_internal.h"

LoadSaveRegistry::LoadSaveRegistry(Explorerplusplus *pContainer) : m_pContainer(pContainer)
{
}

void LoadSaveRegistry::LoadGenericSettings()
{
	m_pContainer->LoadGenericSettingsFromRegistry();
}

void LoadSaveRegistry::LoadBookmarks()
{
	BookmarkRegistryStorage::Load(NExplorerplusplus::REG_MAIN_KEY,
		BookmarkTreeFactory::GetInstance()->GetBookmarkTree());
}

int LoadSaveRegistry::LoadPreviousTabs()
{
	return m_pContainer->LoadTabSettingsFromRegistry();
}

void LoadSaveRegistry::LoadDefaultColumns()
{
	m_pContainer->LoadDefaultColumnsFromRegistry();
}

void LoadSaveRegistry::LoadApplicationToolbar()
{
	Applications::ApplicationToolbarRegistryStorage::Load(NExplorerplusplus::REG_MAIN_KEY,
		Applications::ApplicationModelFactory::GetInstance()->GetApplicationModel());
}

void LoadSaveRegistry::LoadToolbarInformation()
{
	m_pContainer->LoadToolbarInformationFromRegistry();
}

void LoadSaveRegistry::LoadColorRules()
{
	ColorRuleRegistryStorage::Load(NExplorerplusplus::REG_MAIN_KEY,
		ColorRuleModelFactory::GetInstance()->GetColorRuleModel());
}

void LoadSaveRegistry::LoadDialogStates()
{
	DialogHelper::LoadDialogStatesFromRegistry();
}

void LoadSaveRegistry::SaveGenericSettings()
{
	m_pContainer->SaveGenericSettingsToRegistry();
}

void LoadSaveRegistry::SaveBookmarks()
{
	BookmarkRegistryStorage::Save(NExplorerplusplus::REG_MAIN_KEY,
		BookmarkTreeFactory::GetInstance()->GetBookmarkTree());
}

void LoadSaveRegistry::SaveTabs()
{
	m_pContainer->SaveTabSettingsToRegistry();
}

void LoadSaveRegistry::SaveDefaultColumns()
{
	m_pContainer->SaveDefaultColumnsToRegistry();
}

void LoadSaveRegistry::SaveApplicationToolbar()
{
	Applications::ApplicationToolbarRegistryStorage::Save(NExplorerplusplus::REG_MAIN_KEY,
		Applications::ApplicationModelFactory::GetInstance()->GetApplicationModel());
}

void LoadSaveRegistry::SaveToolbarInformation()
{
	m_pContainer->SaveToolbarInformationToRegistry();
}

void LoadSaveRegistry::SaveColorRules()
{
	ColorRuleRegistryStorage::Save(NExplorerplusplus::REG_MAIN_KEY,
		ColorRuleModelFactory::GetInstance()->GetColorRuleModel());
}

void LoadSaveRegistry::SaveDialogStates()
{
	DialogHelper::SaveDialogStatesToRegistry();
}
