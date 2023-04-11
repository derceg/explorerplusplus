// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LoadSaveRegistry.h"
// clang-format off
#include "Explorer++.h"
// clang-format on

LoadSaveRegistry::LoadSaveRegistry(Explorerplusplus *pContainer) : m_pContainer(pContainer)
{
}

void LoadSaveRegistry::LoadGenericSettings()
{
	m_pContainer->LoadGenericSettingsFromRegistry();
}

void LoadSaveRegistry::LoadBookmarks()
{
	m_pContainer->LoadBookmarksFromRegistry();
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
	m_pContainer->LoadApplicationToolbarFromRegistry();
}

void LoadSaveRegistry::LoadToolbarInformation()
{
	m_pContainer->LoadToolbarInformationFromRegistry();
}

void LoadSaveRegistry::LoadColorRules()
{
	m_pContainer->LoadColorRulesFromRegistry();
}

void LoadSaveRegistry::LoadDialogStates()
{
	m_pContainer->LoadDialogStatesFromRegistry();
}

void LoadSaveRegistry::SaveGenericSettings()
{
	m_pContainer->SaveGenericSettingsToRegistry();
}

void LoadSaveRegistry::SaveBookmarks()
{
	m_pContainer->SaveBookmarksToRegistry();
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
	m_pContainer->SaveApplicationToolbarToRegistry();
}

void LoadSaveRegistry::SaveToolbarInformation()
{
	m_pContainer->SaveToolbarInformationToRegistry();
}

void LoadSaveRegistry::SaveColorRules()
{
	m_pContainer->SaveColorRulesToRegistry();
}

void LoadSaveRegistry::SaveDialogStates()
{
	m_pContainer->SaveDialogStatesToRegistry();
}
