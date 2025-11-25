// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Pidl.h"
#include <vector>

class MenuViewFake;

namespace MenuViewFakeTestHelper
{

// For a MenuViewFake that contains a list of shell items, this function verifies that the details
// of each item in the menu match the details of the corresponding pidl.
void CheckItemDetails(MenuViewFake *menuView, const std::vector<PidlAbsolute> &expectedItems);

}
