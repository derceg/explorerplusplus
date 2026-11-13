// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Storage.h"
#include <memory>

class RegistryAppStorage;

class RegistryAppStorageFactory
{
public:
	static std::unique_ptr<RegistryAppStorage> MaybeCreate(const std::wstring &applicationKeyPath,
		Storage::OperationType operationType, HRESULT *error = nullptr);
};
