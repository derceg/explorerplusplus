// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>

struct TabStorageData;

namespace TabRegistryStorage
{

std::vector<TabStorageData> Load(HKEY tabsKey);
void Save(HKEY tabsKey, const std::vector<TabStorageData> &tabs);

}
