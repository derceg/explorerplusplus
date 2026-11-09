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
#include "RebarBandStorageData.h"
#include "TabStorage.h"
#include "WindowRegistryStorage.h"
#include "WindowStorage.h"
#include "../Helper/StorageWriteScope.h"
#include <ktmw32.h>

#pragma comment(lib, "KtmW32.lib")

RegistryAppStorage::RegistryAppStorage(wil::unique_hkey applicationKey,
	wil::unique_handle transaction) :
	m_transaction(std::move(transaction)),
	m_applicationKey(std::move(applicationKey))
{
}

RegistryAppStorage::~RegistryAppStorage()
{
	m_applicationKey.reset();
	if (m_transaction && !m_committed)
		RollbackTransaction(m_transaction.get());
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
	StorageWriteScope writeScope(m_writeError, m_transaction.get());
	ConfigRegistryStorage::Save(m_applicationKey.get(), config);
}

void RegistryAppStorage::SaveWindows(const std::vector<WindowStorageData> &windows)
{
	StorageWriteScope writeScope(m_writeError, m_transaction.get());
	WindowRegistryStorage::Save(m_applicationKey.get(), windows);
}

void RegistryAppStorage::SaveBookmarks(const BookmarkTree *bookmarkTree)
{
	StorageWriteScope writeScope(m_writeError, m_transaction.get());
	BookmarkRegistryStorage::Save(m_applicationKey.get(), bookmarkTree);
}

void RegistryAppStorage::SaveColorRules(const ColorRuleModel *model)
{
	StorageWriteScope writeScope(m_writeError, m_transaction.get());
	ColorRuleRegistryStorage::Save(m_applicationKey.get(), model);
}

void RegistryAppStorage::SaveApplications(const Applications::ApplicationModel *model)
{
	StorageWriteScope writeScope(m_writeError, m_transaction.get());
	Applications::ApplicationToolbarRegistryStorage::Save(m_applicationKey.get(), model);
}

void RegistryAppStorage::SaveDialogStates()
{
	StorageWriteScope writeScope(m_writeError, m_transaction.get());
	DialogStorageHelper::SaveDialogStatesToRegistry(m_applicationKey.get());
}

void RegistryAppStorage::SaveDefaultColumns(const FolderColumns &defaultColumns)
{
	StorageWriteScope writeScope(m_writeError, m_transaction.get());
	DefaultColumnRegistryStorage::Save(m_applicationKey.get(), defaultColumns);
}

void RegistryAppStorage::SaveFrequentLocations(const FrequentLocationsModel *frequentLocationsModel)
{
	StorageWriteScope writeScope(m_writeError, m_transaction.get());
	FrequentLocationsRegistryStorage::Save(m_applicationKey.get(), frequentLocationsModel);
}

HRESULT RegistryAppStorage::Commit()
{
	RETURN_IF_FAILED(m_writeError);
	RETURN_HR_IF(E_UNEXPECTED, !m_transaction || m_committed);
	RETURN_IF_WIN32_BOOL_FALSE(CommitTransaction(m_transaction.get()));
	m_committed = true;
	return S_OK;
}
