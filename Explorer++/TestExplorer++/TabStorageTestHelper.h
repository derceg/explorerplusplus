// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>

struct TabStorageData;

bool operator==(const TabStorageData &first, const TabStorageData &second);

void BuildTabStorageLoadSaveReference(std::vector<TabStorageData> &outputTabs);
