// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellContextMenuDelegate.h"

// Delegates that inherit from this can only be used with the item context menu (i.e. the menu
// that's shown when right-clicking one or more items).
class ShellItemContextMenuDelegate : public ShellContextMenuDelegate
{
};
