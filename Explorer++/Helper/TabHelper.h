// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

namespace TabHelper
{

void SwapItems(HWND tabControl, int item1, int item2);
int MoveItem(HWND tabControl, int currentIndex, int newIndex);
std::wstring GetItemText(HWND tabControl, int item);

}
