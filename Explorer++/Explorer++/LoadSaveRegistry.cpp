// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LoadSaveRegistry.h"
#include "ColorRuleHelper.h"
#include "Explorer++.h"

CLoadSaveRegistry::CLoadSaveRegistry(Explorerplusplus *pContainer) :
	m_pContainer(pContainer)
{

}

void CLoadSaveRegistry::LoadGenericSettings()
{
	m_pContainer->LoadGenericSettingsFromRegistry();
}

void CLoadSaveRegistry::LoadBookmarks()
{
	m_pContainer->LoadBookmarksFromRegistry();
}

int CLoadSaveRegistry::LoadPreviousTabs()
{
	return m_pContainer->LoadTabSettingsFromRegistry();
}

void CLoadSaveRegistry::LoadDefaultColumns()
{
	m_pContainer->LoadDefaultColumnsFromRegistry();
}

void CLoadSaveRegistry::LoadApplicationToolbar()
{
	m_pContainer->LoadApplicationToolbarFromRegistry();
}

void CLoadSaveRegistry::LoadToolbarInformation()
{
	m_pContainer->LoadToolbarInformationFromRegistry();
}

void CLoadSaveRegistry::LoadColorRules()
{
	NColorRuleHelper::LoadColorRulesFromRegistry(m_pContainer->m_ColorRules);
}

void CLoadSaveRegistry::LoadDialogStates()
{
	m_pContainer->LoadDialogStatesFromRegistry();
}

void CLoadSaveRegistry::SaveGenericSettings()
{
	m_pContainer->SaveGenericSettingsToRegistry();
}

void CLoadSaveRegistry::SaveBookmarks()
{
	m_pContainer->SaveBookmarksToRegistry();
}

void CLoadSaveRegistry::SaveTabs()
{
	m_pContainer->SaveTabSettingsToRegistry();
}

void CLoadSaveRegistry::SaveDefaultColumns()
{
	m_pContainer->SaveDefaultColumnsToRegistry();
}

void CLoadSaveRegistry::SaveApplicationToolbar()
{
	m_pContainer->SaveApplicationToolbarToRegistry();
}

void CLoadSaveRegistry::SaveToolbarInformation()
{
	m_pContainer->SaveToolbarInformationToRegistry();
}

void CLoadSaveRegistry::SaveColorRules()
{
	NColorRuleHelper::SaveColorRulesToRegistry(m_pContainer->m_ColorRules);
}

void CLoadSaveRegistry::SaveDialogStates()
{
	m_pContainer->SaveDialogStatesToRegistry();
}