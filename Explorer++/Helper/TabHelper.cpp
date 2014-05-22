/******************************************************************
*
* Project: Helper
* File: TabHelper.cpp
* License: GPL - See LICENSE in the top level directory
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


BOOL TabCtrl_SwapItems(HWND hTabCtrl, int iItem1, int iItem2)
{
	TCITEM tcItem;
	TCHAR szText1[512];
	tcItem.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tcItem.pszText = szText1;
	tcItem.cchTextMax = SIZEOF_ARRAY(szText1);
	BOOL bRet = TabCtrl_GetItem(hTabCtrl, iItem1, &tcItem);

	if(!bRet)
	{
		return FALSE;
	}

	LPARAM lParam1;
	int iImage1;
	lParam1 = tcItem.lParam;
	iImage1 = tcItem.iImage;

	TCHAR szText2[512];
	tcItem.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tcItem.pszText = szText2;
	tcItem.cchTextMax = SIZEOF_ARRAY(szText2);

	bRet = TabCtrl_GetItem(hTabCtrl, iItem2, &tcItem);

	if(!bRet)
	{
		return FALSE;
	}

	LPARAM lParam2;
	int iImage2;
	lParam2 = tcItem.lParam;
	iImage2 = tcItem.iImage;

	tcItem.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tcItem.pszText = szText1;
	tcItem.lParam = lParam1;
	tcItem.iImage = iImage1;
	bRet = TabCtrl_SetItem(hTabCtrl, iItem2, &tcItem);

	if(!bRet)
	{
		return FALSE;
	}

	tcItem.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE;
	tcItem.pszText = szText2;
	tcItem.lParam = lParam2;
	tcItem.iImage = iImage2;
	bRet = TabCtrl_SetItem(hTabCtrl, iItem1, &tcItem);

	if(!bRet)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL TabCtrl_SetItemText(HWND hTabCtrl, int iItem, const TCHAR *pszText)
{
	/* The const_cast below isn't
	particularly good, but is
	required. Information is only
	being set, so the function
	has no reason to modify the
	string. */
	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = const_cast<LPTSTR>(pszText);
	return TabCtrl_SetItem(hTabCtrl, iItem, &tcItem);
}