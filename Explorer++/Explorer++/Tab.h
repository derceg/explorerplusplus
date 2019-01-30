// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/iShellView.h"

struct TabInfo_t
{
	HWND	listView;
	CShellBrowser *shellBrower;
	BOOL	bLocked;
	BOOL	bAddressLocked;
	BOOL	bUseCustomName;
	TCHAR	szName[MAX_PATH];

	/* Although each tab manages its
	own columns, it does not know
	about any column defaults.
	Therefore, it makes more sense
	for this setting to remain here. */
	//BOOL	bUsingDefaultColumns;
};