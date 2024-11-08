// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DefaultColumnRegistryStorage.h"
#include "ColumnRegistryStorage.h"
#include <wil/registry.h>

namespace
{

const wchar_t DEFAULT_COLUMNS_KEY_PATH[] = L"DefaultColumns";

}

namespace DefaultColumnRegistryStorage
{

void Load(HKEY applicationKey, FolderColumns &defaultColumns)
{
	wil::unique_hkey defaultColumnsKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(applicationKey, DEFAULT_COLUMNS_KEY_PATH,
		defaultColumnsKey, wil::reg::key_access::read);

	if (FAILED(hr))
	{
		return;
	}

	ColumnRegistryStorage::LoadAllColumnSets(defaultColumnsKey.get(), defaultColumns);
}

void Save(HKEY applicationKey, const FolderColumns &defaultColumns)
{
	wil::unique_hkey defaultColumnsKey;
	HRESULT hr = wil::reg::create_unique_key_nothrow(applicationKey, DEFAULT_COLUMNS_KEY_PATH,
		defaultColumnsKey, wil::reg::key_access::readwrite);

	if (FAILED(hr))
	{
		return;
	}

	ColumnRegistryStorage::SaveAllColumnSets(defaultColumnsKey.get(), defaultColumns);
}

}
