// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>

struct RebarBandStorageData;

namespace RebarRegistryStorage
{

std::vector<RebarBandStorageData> Load(HKEY rebarKey);
void Save(HKEY rebarKey, const std::vector<RebarBandStorageData> &rebarStorageInfo);

}
