/******************************************************************
 *
 * Project: Helper
 * File: ListViewHelper.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Listview helper functionality.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "ListViewHelper.h"
#include "Macros.h"


void NListView::ListView_SelectItem(HWND hListView,int iItem,BOOL bSelect)
{
	UINT uNewState;

	if(bSelect)
	{
		uNewState = LVIS_SELECTED;
	}
	else
	{
		uNewState = 0;
	}

	ListView_SetItemState(hListView,iItem,uNewState,LVIS_SELECTED);
}

void NListView::ListView_SelectAllItems(HWND hListView,BOOL bSelect)
{
	UINT uNewState;

	if(bSelect)
	{
		uNewState = LVIS_SELECTED;
	}
	else
	{
		uNewState = 0;
	}

	SendMessage(hListView,WM_SETREDRAW,FALSE,0);
	ListView_SetItemState(hListView,-1,uNewState,LVIS_SELECTED);
	SendMessage(hListView,WM_SETREDRAW,TRUE,0);
}

int NListView::ListView_InvertSelection(HWND hListView)
{
	int nTotalItems = ListView_GetItemCount(hListView);

	int nSelected = 0;

	SendMessage(hListView,WM_SETREDRAW,FALSE,0);

	for(int i = 0;i < nTotalItems;i++)
	{
		if(ListView_GetItemState(hListView,i,LVIS_SELECTED) == LVIS_SELECTED)
		{
			ListView_SelectItem(hListView,i,FALSE);
		}
		else
		{
			ListView_SelectItem(hListView,i,TRUE);
			nSelected++;
		}
	}

	SendMessage(hListView,WM_SETREDRAW,TRUE,0);

	return nSelected;
}

void NListView::ListView_FocusItem(HWND hListView,int iItem,BOOL bFocus)
{
	UINT uNewState;

	if(bFocus)
	{
		uNewState = LVIS_FOCUSED;
	}
	else
	{
		uNewState = 0;
	}

	ListView_SetItemState(hListView,iItem,uNewState,LVIS_FOCUSED);
}

void NListView::ListView_SetGridlines(HWND hListView,BOOL bEnableGridlines)
{
	DWORD dwExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	if(bEnableGridlines)
	{
		if((dwExtendedStyle & LVS_EX_GRIDLINES) != LVS_EX_GRIDLINES)
		{
			dwExtendedStyle |= LVS_EX_GRIDLINES;
		}
	}
	else
	{
		if((dwExtendedStyle & LVS_EX_GRIDLINES) == LVS_EX_GRIDLINES)
		{
			dwExtendedStyle &= ~LVS_EX_GRIDLINES;
		}
	}

	ListView_SetExtendedListViewStyle(hListView,dwExtendedStyle);
}

void NListView::ListView_SetAutoArrange(HWND hListView,BOOL bAutoArrange)
{
	LONG_PTR lStyle = GetWindowLongPtr(hListView,GWL_STYLE);

	if(bAutoArrange)
	{
		if((lStyle & LVS_AUTOARRANGE) != LVS_AUTOARRANGE)
		{
			lStyle |= LVS_AUTOARRANGE;
		}
	}
	else
	{
		if((lStyle & LVS_AUTOARRANGE) == LVS_AUTOARRANGE)
		{
			lStyle &= ~LVS_AUTOARRANGE;
		}
	}

	SetWindowLongPtr(hListView,GWL_STYLE,lStyle);
}

void NListView::ListView_ActivateOneClickSelect(HWND hListView,BOOL bActivate,UINT uHoverTime)
{
	DWORD dwExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	/* The three styles below are used to control one-click
	selection. */
	if(bActivate)
	{
		if((dwExtendedStyle & LVS_EX_TRACKSELECT) != LVS_EX_TRACKSELECT)
		{
			dwExtendedStyle |= LVS_EX_TRACKSELECT;
		}

		if((dwExtendedStyle & LVS_EX_ONECLICKACTIVATE) != LVS_EX_ONECLICKACTIVATE)
		{
			dwExtendedStyle |= LVS_EX_ONECLICKACTIVATE;
		}

		if((dwExtendedStyle & LVS_EX_UNDERLINEHOT) != LVS_EX_UNDERLINEHOT)
		{
			dwExtendedStyle |= LVS_EX_UNDERLINEHOT;
		}

		ListView_SetExtendedListViewStyle(hListView,dwExtendedStyle);
		ListView_SetHoverTime(hListView,uHoverTime);
	}
	else
	{
		if((dwExtendedStyle & LVS_EX_TRACKSELECT) == LVS_EX_TRACKSELECT)
		{
			dwExtendedStyle &= ~LVS_EX_TRACKSELECT;
		}

		if((dwExtendedStyle & LVS_EX_ONECLICKACTIVATE) == LVS_EX_ONECLICKACTIVATE)
		{
			dwExtendedStyle &= ~LVS_EX_ONECLICKACTIVATE;
		}

		if((dwExtendedStyle & LVS_EX_UNDERLINEHOT) == LVS_EX_UNDERLINEHOT)
		{
			dwExtendedStyle &= ~LVS_EX_UNDERLINEHOT;
		}

		ListView_SetExtendedListViewStyle(hListView,dwExtendedStyle);
	}
}

void NListView::ListView_AddRemoveExtendedStyle(HWND hListView,DWORD dwStyle,BOOL bAdd)
{
	DWORD dwExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	if(bAdd)
	{
		if((dwExtendedStyle & dwStyle) != dwStyle)
		{
			dwExtendedStyle |= dwStyle;
		}
	}
	else
	{
		if((dwExtendedStyle & dwStyle) == dwStyle)
		{
			dwExtendedStyle &= ~dwStyle;
		}
	}

	ListView_SetExtendedListViewStyle(hListView,dwExtendedStyle);
}

void NListView::ListView_SetBackgroundImage(HWND hListView,UINT uImage)
{
	TCHAR szModuleName[MAX_PATH];
	GetModuleFileName(NULL,szModuleName,SIZEOF_ARRAY(szModuleName));

	LVBKIMAGE lvbki;
	lvbki.ulFlags = LVBKIF_STYLE_NORMAL|LVBKIF_SOURCE_URL;
	lvbki.xOffsetPercent = 45;
	lvbki.yOffsetPercent = 50;

	TCHAR szBitmap[512];

	if(uImage == 0)
	{
		lvbki.pszImage = NULL;
	}
	else
	{
		/* 2 means an image resource, 3 would mean an icon. */
		StringCchPrintf(szBitmap,SIZEOF_ARRAY(szBitmap),
			_T("res://%s/#2/#%d"),szModuleName,uImage);

		lvbki.pszImage = szBitmap;
	}

	ListView_SetBkImage(hListView,&lvbki);
}

void NListView::ListView_SwapItems(HWND hListView,int iItem1,int iItem2)
{
	LVITEM lvItem;
	LPARAM lParam1;
	LPARAM lParam2;
	TCHAR szText1[512];
	TCHAR szText2[512];
	BOOL bItem1Checked;
	BOOL bItem2Checked;
	UINT Item1StateMask;
	UINT Item2StateMask;
	BOOL res;

	lvItem.mask			= LVIF_TEXT | LVIF_PARAM;
	lvItem.iItem		= iItem1;
	lvItem.iSubItem		= 0;
	lvItem.pszText		= szText1;
	lvItem.cchTextMax	= SIZEOF_ARRAY(szText1);

	res = ListView_GetItem(hListView,&lvItem);

	if(!res)
	{
		return;
	}

	lParam1 = lvItem.lParam;

	lvItem.mask			= LVIF_TEXT | LVIF_PARAM;
	lvItem.iItem		= iItem2;
	lvItem.iSubItem		= 0;
	lvItem.pszText		= szText2;
	lvItem.cchTextMax	= SIZEOF_ARRAY(szText2);

	res = ListView_GetItem(hListView,&lvItem);

	if(!res)
	{
		return;
	}

	lParam2 = lvItem.lParam;

	lvItem.mask		= LVIF_TEXT | LVIF_PARAM;
	lvItem.iItem	= iItem1;
	lvItem.iSubItem	= 0;
	lvItem.pszText	= szText2;
	lvItem.lParam	= lParam2;
	ListView_SetItem(hListView,&lvItem);

	lvItem.mask		= LVIF_TEXT | LVIF_PARAM;
	lvItem.iItem	= iItem2;
	lvItem.iSubItem	= 0;
	lvItem.pszText	= szText1;
	lvItem.lParam	= lParam1;
	ListView_SetItem(hListView,&lvItem);

	/* Swap all sub-items. */
	HWND hHeader;
	TCHAR szBuffer1[512];
	TCHAR szBuffer2[512];
	int nColumns;
	int iSubItem = 1;
	int i = 0;

	hHeader = ListView_GetHeader(hListView);

	nColumns = Header_GetItemCount(hHeader);

	for(i = 1;i < nColumns;i++)
	{
		lvItem.mask			= LVIF_TEXT;
		lvItem.iItem		= iItem1;
		lvItem.iSubItem		= iSubItem;
		lvItem.pszText		= szBuffer1;
		lvItem.cchTextMax	= SIZEOF_ARRAY(szBuffer1);

		res = ListView_GetItem(hListView,&lvItem);

		lvItem.mask			= LVIF_TEXT;
		lvItem.iItem		= iItem2;
		lvItem.iSubItem		= iSubItem;
		lvItem.pszText		= szBuffer2;
		lvItem.cchTextMax	= SIZEOF_ARRAY(szBuffer2);

		res = ListView_GetItem(hListView,&lvItem);

		lvItem.mask		= LVIF_TEXT;
		lvItem.iItem	= iItem1;
		lvItem.iSubItem	= iSubItem;
		lvItem.pszText	= szBuffer2;
		ListView_SetItem(hListView,&lvItem);

		lvItem.mask		= LVIF_TEXT;
		lvItem.iItem	= iItem2;
		lvItem.iSubItem	= iSubItem;
		lvItem.pszText	= szBuffer1;
		ListView_SetItem(hListView,&lvItem);

		iSubItem++;
	}

	Item1StateMask = ListView_GetItemState(hListView,iItem1,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);
	Item2StateMask = ListView_GetItemState(hListView,iItem2,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);

	ListView_SetItemState(hListView,iItem1,Item2StateMask,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);
	ListView_SetItemState(hListView,iItem2,Item1StateMask,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);

	bItem1Checked = ListView_GetCheckState(hListView,iItem1);
	bItem2Checked = ListView_GetCheckState(hListView,iItem2);

	ListView_SetCheckState(hListView,iItem1,bItem2Checked);
	ListView_SetCheckState(hListView,iItem2,bItem1Checked);
}

void NListView::ListView_SwapItemsNolParam(HWND hListView,int iItem1,int iItem2)
{
	LVITEM lvItem;
	TCHAR szText1[512];
	TCHAR szText2[512];
	BOOL bItem1Checked;
	BOOL bItem2Checked;
	UINT Item1StateMask;
	UINT Item2StateMask;
	BOOL res;

	lvItem.mask			= LVIF_TEXT;
	lvItem.iItem		= iItem1;
	lvItem.iSubItem		= 0;
	lvItem.pszText		= szText1;
	lvItem.cchTextMax	= SIZEOF_ARRAY(szText1);

	res = ListView_GetItem(hListView,&lvItem);

	if(!res)
	{
		return;
	}

	lvItem.mask			= LVIF_TEXT;
	lvItem.iItem		= iItem2;
	lvItem.iSubItem		= 0;
	lvItem.pszText		= szText2;
	lvItem.cchTextMax	= SIZEOF_ARRAY(szText2);

	res = ListView_GetItem(hListView,&lvItem);

	if(!res)
	{
		return;
	}

	lvItem.mask		= LVIF_TEXT;
	lvItem.iItem	= iItem1;
	lvItem.iSubItem	= 0;
	lvItem.pszText	= szText2;
	ListView_SetItem(hListView,&lvItem);

	lvItem.mask		= LVIF_TEXT;
	lvItem.iItem	= iItem2;
	lvItem.iSubItem	= 0;
	lvItem.pszText	= szText1;
	ListView_SetItem(hListView,&lvItem);

	/* Swap all sub-items. */
	HWND hHeader;
	TCHAR szBuffer1[512];
	TCHAR szBuffer2[512];
	int nColumns;
	int iSubItem = 1;
	int i = 0;

	hHeader = ListView_GetHeader(hListView);

	nColumns = Header_GetItemCount(hHeader);

	for(i = 0;i < nColumns;i++)
	{
		lvItem.mask			= LVIF_TEXT;
		lvItem.iItem		= iItem1;
		lvItem.iSubItem		= iSubItem;
		lvItem.pszText		= szBuffer1;
		lvItem.cchTextMax	= SIZEOF_ARRAY(szBuffer1);

		res = ListView_GetItem(hListView,&lvItem);

		lvItem.mask			= LVIF_TEXT;
		lvItem.iItem		= iItem2;
		lvItem.iSubItem		= iSubItem;
		lvItem.pszText		= szBuffer2;
		lvItem.cchTextMax	= SIZEOF_ARRAY(szBuffer2);

		res = ListView_GetItem(hListView,&lvItem);

		lvItem.mask		= LVIF_TEXT;
		lvItem.iItem	= iItem1;
		lvItem.iSubItem	= iSubItem;
		lvItem.pszText	= szBuffer2;
		ListView_SetItem(hListView,&lvItem);

		lvItem.mask		= LVIF_TEXT;
		lvItem.iItem	= iItem2;
		lvItem.iSubItem	= iSubItem;
		lvItem.pszText	= szBuffer1;
		ListView_SetItem(hListView,&lvItem);

		iSubItem++;
	}

	Item1StateMask = ListView_GetItemState(hListView,iItem1,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);
	Item2StateMask = ListView_GetItemState(hListView,iItem2,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);

	ListView_SetItemState(hListView,iItem1,Item2StateMask,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);
	ListView_SetItemState(hListView,iItem2,Item1StateMask,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);

	bItem1Checked = ListView_GetCheckState(hListView,iItem1);
	bItem2Checked = ListView_GetCheckState(hListView,iItem2);

	ListView_SetCheckState(hListView,iItem1,bItem2Checked);
	ListView_SetCheckState(hListView,iItem2,bItem1Checked);
}

void NListView::ListView_HandleInsertionMark(HWND hListView,int iItemFocus,POINT *ppt)
{
	/* Remove the insertion mark. */
	if(ppt == NULL)
	{
		LVINSERTMARK lvim;
		lvim.cbSize		= sizeof(LVINSERTMARK);
		lvim.dwFlags	= 0;
		lvim.iItem		= -1;
		ListView_SetInsertMark(hListView,&lvim);

		return;
	}

	RECT ItemRect;
	DWORD dwFlags = 0;
	int iNext;

	LV_HITTESTINFO item;
	item.pt = *ppt;

	int iItem = ListView_HitTest(hListView,&item);

	if(iItem != -1 && item.flags & LVHT_ONITEM)
	{
		ListView_GetItemRect(hListView,item.iItem,&ItemRect,LVIR_BOUNDS);

		/* If the cursor is on the left side
		of this item, set the insertion before
		this item; if it is on the right side
		of this item, set the insertion mark
		after this item. */
		if((ppt->x - ItemRect.left) >
			((ItemRect.right - ItemRect.left)/2))
		{
			iNext = iItem;
			dwFlags = LVIM_AFTER;
		}
		else
		{
			iNext = iItem;
			dwFlags = 0;
		}
	}
	else
	{
		dwFlags = 0;

		/* VK_UP finds whichever item is "above" the cursor.
		This item may be in the same row as the cursor is in
		(e.g. an item in the next row won't be found until the
		cursor passes into that row). Appears to find the item
		on the right side. */
		LVFINDINFO lvfi;
		lvfi.flags			= LVFI_NEARESTXY;
		lvfi.pt				= *ppt;
		lvfi.vkDirection	= VK_UP;
		iNext = ListView_FindItem(hListView,-1,&lvfi);

		if(iNext == -1)
		{
			lvfi.flags			= LVFI_NEARESTXY;
			lvfi.pt				= *ppt;
			lvfi.vkDirection	= VK_LEFT;
			iNext = ListView_FindItem(hListView,-1,&lvfi);
		}

		ListView_GetItemRect(hListView,iNext,&ItemRect,LVIR_BOUNDS);

		/* This situation only occurs at the
		end of the row. Prior to this, it is
		always the item on the right side that
		is found, with the insertion mark been
		inserted before that item.
		Once the end of the row is reached, the
		item found will be on the left side of
		the cursor. */
		if(ppt->x > ItemRect.left +
			((ItemRect.right - ItemRect.left)/2))
		{
			/* At the end of a row, VK_UP appears to
			find the next item up. Therefore, if we're
			at the end of a row, and the cursor is
			completely below the "next" item, find the
			one under it instead (if there is an item
			under it), and anchor the insertion mark
			there. */
			if(ppt->y > ItemRect.bottom)
			{
				int iBelow;

				iBelow = ListView_GetNextItem(hListView,iNext,LVNI_BELOW);

				if(iBelow != -1)
					iNext = iBelow;
			}

			dwFlags = LVIM_AFTER;
		}

		int nItems = ListView_GetItemCount(hListView);

		/* Last item is at position nItems - 1. */
		ListView_GetItemRect(hListView,nItems - 1,&ItemRect,LVIR_BOUNDS);

		/* Special case needed for very last item. If cursor is within 0.5 to 1.5 width
		of last item, and is greater than it's y coordinate, snap the insertion mark to
		this item. */
		if((ppt->x > ItemRect.left + ((ItemRect.right - ItemRect.left)/2)) &&
			ppt->x < ItemRect.right + ((ItemRect.right - ItemRect.left)/2) + 2 &&
			ppt->y > ItemRect.top)
		{
			iNext = nItems - 1;
			dwFlags = LVIM_AFTER;
		}
	}

	LVINSERTMARK lvim;
	lvim.cbSize			= sizeof(LVINSERTMARK);
	lvim.dwFlags		= dwFlags;
	lvim.iItem			= iNext;
	ListView_SetInsertMark(hListView,&lvim);
}