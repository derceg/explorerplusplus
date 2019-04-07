// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ImageWrappers.h"

HImageListPtr GetShellImageList();
void SetMenuItemImageFromImageList(HMENU menu, UINT menuItemId, HIMAGELIST imageList, int bitmapIndex, std::vector<HBitmapPtr> &menuImages);