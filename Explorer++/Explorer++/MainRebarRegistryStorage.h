// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>

struct RebarBandStorageInfo;

namespace MainRebarRegistryStorage
{

std::vector<RebarBandStorageInfo> Load(HKEY mainKey);
void Save(HKEY mainKey, const std::vector<RebarBandStorageInfo> &rebarStorageInfo);

}
