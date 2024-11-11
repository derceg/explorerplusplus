// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RegistryAppStorageFactory.h"
#include "RegistryAppStorage.h"
#include "Storage.h"
#include <wil/registry.h>

std::unique_ptr<RegistryAppStorage> RegistryAppStorageFactory::MaybeCreate()
{
	wil::unique_hkey applicationKey;
	HRESULT hr = wil::reg::open_unique_key_nothrow(HKEY_CURRENT_USER,
		Storage::REGISTRY_APPLICATION_KEY_PATH, applicationKey, wil::reg::key_access::read);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return std::make_unique<RegistryAppStorage>(std::move(applicationKey));
}
