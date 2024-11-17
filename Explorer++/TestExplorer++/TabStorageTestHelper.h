// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "StorageTestHelper.h"
#include <vector>

struct TabStorageData;

TabStorageData CreateTabStorageFromDirectory(const std::wstring &directory,
	TestStorageType storageType);
void BuildTabStorageLoadSaveReference(std::vector<TabStorageData> &outputTabs,
	TestStorageType storageType);
