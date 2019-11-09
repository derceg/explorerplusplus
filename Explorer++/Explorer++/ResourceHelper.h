// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <unordered_map>
#include <tuple>

void SetMenuItemImage(HMENU menu, UINT menuItemId, UINT imageResourceId, std::vector<wil::unique_hbitmap> &menuImages);
std::tuple<wil::unique_himagelist, std::unordered_map<UINT, int>> CreateIconImageList(int iconSize, const std::initializer_list<UINT> &resourceIds);
void AddIconToImageList(HIMAGELIST imageList, UINT resourceId, std::unordered_map<UINT, int> &imageListMappings);