// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "StringHelper.h"
#include <optional>
#include <string>

namespace ListViewHelper
{

void SelectItem(HWND hListView, int iItem, BOOL bSelect);
void SelectAllItems(HWND hListView, BOOL bSelect);
int InvertSelection(HWND hListView);
void FocusItem(HWND hListView, int iItem, BOOL bFocus);
void SetGridlines(HWND hListView, BOOL bEnableGridlines);
BOOL SetAutoArrange(HWND hListView, BOOL bAutoArrange);
void ActivateOneClickSelect(HWND hListView, BOOL bActivate, UINT uHoverTime);
void AddRemoveExtendedStyle(HWND hListView, DWORD dwStyle, BOOL bAdd);
BOOL SwapItems(HWND hListView, int iItem1, int iItem2, BOOL bSwapLPARAM);
void PositionInsertMark(HWND hListView, const POINT *ppt);
std::optional<int> GetLastSelectedItemIndex(HWND listView);
std::wstring GetItemText(HWND listView, int item, int subItem = 0);

// Returns true if the listview contains the specified text. Both the column headers and item rows
// are searched.
bool DoesListViewContainText(HWND listView, const std::wstring &text,
	StringComparatorFunc stringComparator);

}
