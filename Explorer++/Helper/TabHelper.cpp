/******************************************************************
*
* Project: Helper
* File: TabHelper.cpp
* License: GPL - See COPYING in the top level directory
*
* Tab helper functionality.
*
* Written by David Erceg
* www.explorerplusplus.com
*
*****************************************************************/

#include "stdafx.h"
#include "TabHelper.h"
#include "Macros.h"


void TabCtrl_SwapItems(HWND hTabCtrl, int iItem1, int iItem2)
{
	TCITEM tcItem;
	LPARAM lParam1;
	LPARAM lParam2;
	TCHAR szText1[512];
	TCHAR szText2[512];
	int iImage1;
	int iImage2;
	BOOL res;

	tcItem.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tcItem.pszText = szText1;
	tcItem.cchTextMax = SIZEOF_ARRAY(szText1);

	res = TabCtrl_GetItem(hTabCtrl, iItem1, &tcItem);

	if(!res)
		return;

	lParam1 = tcItem.lParam;
	iImage1 = tcItem.iImage;

	tcItem.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tcItem.pszText = szText2;
	tcItem.cchTextMax = SIZEOF_ARRAY(szText2);

	res = TabCtrl_GetItem(hTabCtrl, iItem2, &tcItem);

	if(!res)
		return;

	lParam2 = tcItem.lParam;
	iImage2 = tcItem.iImage;

	tcItem.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tcItem.pszText = szText1;
	tcItem.lParam = lParam1;
	tcItem.iImage = iImage1;

	TabCtrl_SetItem(hTabCtrl, iItem2, &tcItem);

	tcItem.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tcItem.pszText = szText2;
	tcItem.lParam = lParam2;
	tcItem.iImage = iImage2;

	TabCtrl_SetItem(hTabCtrl, iItem1, &tcItem);
}

void TabCtrl_SetItemText(HWND Tab, int iTab, TCHAR *Text)
{
	TCITEM tcItem;

	if(Text == NULL)
		return;

	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = Text;

	SendMessage(Tab, TCM_SETITEM, iTab, reinterpret_cast<LPARAM>(&tcItem));
}