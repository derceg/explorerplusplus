// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>

namespace MenuHelper
{
void AddStringItem(HMENU menu, UINT id, std::wstring &text);
void AddStringItem(HMENU menu, UINT id, std::wstring &text, UINT item, BOOL byPosition);
void AddSeparator(HMENU menu);
void AddSeparator(HMENU menu, UINT item, BOOL byPosition);
void AttachSubMenu(HMENU parentMenu, wil::unique_hmenu subMenu, UINT item, BOOL byPosition);

void CheckItem(HMENU hMenu, UINT itemID, BOOL bCheck);
void EnableItem(HMENU hMenu, UINT itemID, BOOL bEnable);
}