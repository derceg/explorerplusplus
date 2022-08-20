// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

BOOL TabCtrl_SwapItems(HWND hTabCtrl, int iItem1, int iItem2);
int TabCtrl_MoveItem(HWND tabCtrl, int currentIndex, int newIndex);
BOOL TabCtrl_SetItemText(HWND hTabCtrl, int iItem, const TCHAR *pszText);
