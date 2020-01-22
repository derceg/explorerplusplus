// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuHelper.h"


void lCheckMenuItem(HMENU hMenu, UINT ItemID, BOOL bCheck)
{
	UINT state = bCheck ? MF_CHECKED : MF_UNCHECKED;
	CheckMenuItem(hMenu, ItemID, state);
}

void lEnableMenuItem(HMENU hMenu, UINT ItemID, BOOL bEnable)
{
	UINT state = bEnable ? MF_ENABLED : MF_DISABLED;
	EnableMenuItem(hMenu, ItemID, state);
}