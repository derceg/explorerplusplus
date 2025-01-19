// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "StringHelper.h"
#include <optional>
#include <set>
#include <string>

namespace ListViewHelper
{

void SelectItem(HWND listView, int item, bool select);
void SelectAllItems(HWND listView, bool select);
void InvertSelection(HWND listView);
void FocusItem(HWND listView, int item, bool focus);
void SetAutoArrange(HWND listView, bool autoArrange);
void ActivateOneClickSelect(HWND listView, bool activate, UINT hoverTime);
void AddRemoveExtendedStyles(HWND listView, DWORD styles, bool add);
void SwapItems(HWND listView, int item1, int item2);
void PositionInsertMark(HWND hListView, const POINT *ppt);
std::optional<int> GetLastSelectedItemIndex(HWND listView);
std::wstring GetItemText(HWND listView, int item, int subItem = 0);

// Returns true if the listview contains the specified text. Both the column headers and item rows
// are searched.
bool DoesListViewContainText(HWND listView, const std::wstring &text,
	StringComparatorFunc stringComparator);

std::set<int> GetSelectedItems(HWND listView);

}
