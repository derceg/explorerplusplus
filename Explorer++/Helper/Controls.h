// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "StringHelper.h"

enum class TooltipType
{
	Control,
	Rectangle
};

HWND CreateListView(HWND hParent, DWORD dwStyle);
HWND CreateTreeView(HWND hParent, DWORD dwStyle);
HWND CreateStatusBar(HWND hParent, DWORD dwStyle);
HWND CreateToolbar(HWND hParent, DWORD dwStyle, DWORD dwExStyle);
HWND CreateTabControl(HWND hParent, DWORD dwStyle);
HWND CreateTooltipControl(HWND parent, HINSTANCE resourceInstance);
BOOL AddPathsToComboBoxEx(HWND hComboBoxEx, const TCHAR *path);

/* Dialog. */
BOOL lCheckDlgButton(HWND hDlg, int buttonId, BOOL bCheck);

/* Toolbar. */
void RefreshToolbarAfterFontOrDpiChange(HWND toolbar);

SIZE GetCheckboxSize(HWND hwnd);
SIZE GetRadioButtonSize(HWND hwnd);
SIZE GetButtonSize(HWND hwnd, int partId, int stateId, int defaultWidth, int defaultHeight);

void AddTooltipForControl(HWND tipWnd, HWND control, const std::wstring &tooltip,
	TooltipType tooltipType = TooltipType::Control);

struct ComboBoxItem
{
	int id;
	std::wstring text;
};

void AddItemsToComboBox(HWND comboBox, const std::vector<ComboBoxItem> &items, int currentItemId);

// Returns true if an item within the list box of the specified combo box contains the text that's
// provided.
bool DoesComboBoxContainText(HWND comboBox, const std::wstring &text,
	StringComparatorFunc stringComparator);
