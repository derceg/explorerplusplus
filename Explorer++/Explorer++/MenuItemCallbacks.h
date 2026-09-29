// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <functional>

using MenuItemEnabledCallback = std::function<bool()>;
using MenuItemCheckedCallback = std::function<bool()>;
using MenuItemSelectedCallback = std::function<void(bool isCtrlKeyDown, bool isShiftKeyDown)>;
using MenuItemMiddleClickedCallback = std::function<void(bool isCtrlKeyDown, bool isShiftKeyDown)>;
