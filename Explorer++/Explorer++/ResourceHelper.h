// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>

void SetMenuItemImage(HMENU menu, UINT menuItemId, UINT imageResourceId, std::vector<wil::unique_hbitmap> &menuImages);