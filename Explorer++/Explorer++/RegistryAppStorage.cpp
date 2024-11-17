// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RegistryAppStorage.h"
#include "ApplicationToolbarRegistryStorage.h"
#include "Bookmarks/BookmarkRegistryStorage.h"
#include "ColorRuleRegistryStorage.h"
#include "DefaultColumnRegistryStorage.h"
#include "DialogHelper.h"
#include "MainRebarStorage.h"
#include "TabStorage.h"
#include "WindowRegistryStorage.h"
#include "WindowStorage.h"

RegistryAppStorage::RegistryAppStorage(wil::unique_hkey applicationKey) :
	m_applicationKey(std::move(applicationKey))
{
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
	DialogHelper::LoadDialogStatesFromRegistry(m_applicationKey.get());
}

void RegistryAppStorage::LoadDefaultColumns(FolderColumns &defaultColumns)
{
	DefaultColumnRegistryStorage::Load(m_applicationKey.get(), defaultColumns);
}
