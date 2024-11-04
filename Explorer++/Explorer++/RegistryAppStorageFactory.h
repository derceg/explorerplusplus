// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>

class RegistryAppStorage;

class RegistryAppStorageFactory
{
public:
	static std::unique_ptr<RegistryAppStorage> MaybeCreate();
};
