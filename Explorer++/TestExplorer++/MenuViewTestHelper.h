// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Pidl.h"
#include <vector>

class MenuView;

namespace MenuViewTestHelper
{

// For a MenuView that contains a list of shell items, this function verifies that the details of
// each item in the menu match the details of the corresponding pidl.
void CheckShellItemDetails(MenuView *menuView, const std::vector<PidlAbsolute> &expectedItems);

void ExpectItemEnabled(const MenuView *menuView, UINT id, bool enabled);
void ExpectItemChecked(const MenuView *menuView, UINT id, bool checked);

}
