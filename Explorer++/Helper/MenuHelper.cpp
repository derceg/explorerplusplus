/******************************************************************
*
* Project: Helper
* File: MenuHelper.cpp
* License: GPL - See LICENSE in the top level directory
*
* Menu helper functionality.
*
* Written by David Erceg
* www.explorerplusplus.com
*
*****************************************************************/

#include "stdafx.h"
#include "MenuHelper.h"


void lCheckMenuItem(HMENU hMenu, UINT ItemID, BOOL bCheck)
{
	UINT state = bCheck ? MF_CHECKED : MF_UNCHECKED;
	CheckMenuItem(hMenu, ItemID, state);
}

void lEnableMenuItem(HMENU hMenu, UINT ItemID, BOOL bEnable)
{
	UINT state = bEnable ? MF_ENABLED : MF_GRAYED;
	EnableMenuItem(hMenu, ItemID, state);
}