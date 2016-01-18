/******************************************************************
*
* Project: Explorer++
* File: CustomMenu.cpp
* License: GPL - See LICENSE in the top level directory
*
* Custom menu functionality.
*
* Written by David Erceg
* www.explorerplusplus.com
*
*****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"


void Explorerplusplus::SetMenuOwnerDraw(HMENU hMenu)
{
	int nTopLevelMenus;

	nTopLevelMenus = GetMenuItemCount(hMenu);

	SetMenuOwnerDrawInternal(hMenu, nTopLevelMenus);
}

/*
* Marks the specified menu as owner drawn.
*/
void Explorerplusplus::SetMenuOwnerDrawInternal(HMENU hMenu,
	int nMenus)
{
	MENUITEMINFO		mi;
	int					i = 0;

	for(i = 0; i < nMenus; i++)
	{
		SetMenuItemOwnerDrawn(hMenu, i);

		mi.cbSize = sizeof(mi);
		mi.fMask = MIIM_SUBMENU;
		GetMenuItemInfo(hMenu, i, TRUE, &mi);

		if(mi.hSubMenu != NULL)
			SetMenuOwnerDraw(mi.hSubMenu);
	}
}

void Explorerplusplus::SetMenuItemOwnerDrawn(HMENU hMenu, int iItem)
{
	MENUITEMINFO		mi;
	CustomMenuInfo_t	*pcmi = NULL;

	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_FTYPE | MIIM_ID;

	GetMenuItemInfo(hMenu, iItem, TRUE, &mi);

	if(!(mi.fType & MFT_OWNERDRAW))
		mi.fType |= MFT_OWNERDRAW;

	pcmi = (CustomMenuInfo_t *) malloc(sizeof(CustomMenuInfo_t));

	pcmi->bUseImage = FALSE;
	pcmi->dwItemData = 0;

	mi.fMask |= MIIM_DATA;
	mi.dwItemData = (ULONG_PTR) pcmi;
	SetMenuItemInfo(hMenu, iItem, TRUE, &mi);
}

/*
* Sets the bitmap for an owner drawn menu.
* The menu MUST have already been marked as
* owner drawn (so that the owner drawn menu
* structure is in place).
*/
void Explorerplusplus::SetMenuItemBitmap(HMENU hMenu, UINT ItemID, int iBitmap)
{
	MENUITEMINFO		mi;
	CustomMenuInfo_t	*pcmi = NULL;

	BOOL bRes;

	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_DATA;
	bRes = GetMenuItemInfo(hMenu, ItemID, FALSE, &mi);

	if(bRes)
	{
		pcmi = (CustomMenuInfo_t *) mi.dwItemData;

		pcmi->bUseImage = TRUE;
		pcmi->iImage = iBitmap;
	}
}