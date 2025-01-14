// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ListViewHelper.h"
#include "Macros.h"

namespace
{

BOOL GetListViewItem(HWND hListView, LVITEM *pLVItem, UINT mask, UINT stateMask, int iItem,
	int iSubItem, TCHAR *pszText, int cchMax)
{
	pLVItem->mask = mask;
	pLVItem->stateMask = stateMask;
	pLVItem->iItem = iItem;
	pLVItem->iSubItem = iSubItem;

	if (mask & LVIF_TEXT)
	{
		pLVItem->pszText = pszText;
		pLVItem->cchTextMax = cchMax;
	}

	return ListView_GetItem(hListView, pLVItem);
}

bool DoesHeaderContainText(HWND listView, const std::wstring &text,
	StringComparatorFunc stringComparator)
{
	HWND header = ListView_GetHeader(listView);
	int numColumns = Header_GetItemCount(header);

	for (int i = 0; i < numColumns; i++)
	{
		wchar_t columnText[260];

		LVCOLUMN lvColumn = {};
		lvColumn.mask = LVCF_TEXT;
		lvColumn.pszText = columnText;
		lvColumn.cchTextMax = std::size(columnText);
		auto res = ListView_GetColumn(listView, i, &lvColumn);

		if (!res)
		{
			DCHECK(false);
			continue;
		}

		if (stringComparator(columnText, text))
		{
			return true;
		}
	}

	return false;
}

bool DoesItemRowContainText(HWND listView, int item, const std::wstring &text,
	StringComparatorFunc stringComparator)
{
	HWND header = ListView_GetHeader(listView);
	int numColumns = Header_GetItemCount(header);

	for (int i = 0; i < numColumns; i++)
	{
		auto itemColumnText = ListViewHelper::GetItemText(listView, item, i);

		if (stringComparator(itemColumnText, text))
		{
			return true;
		}
	}

	return false;
}

bool DoAnyItemsContainText(HWND listView, const std::wstring &text,
	StringComparatorFunc stringComparator)
{
	int numItems = ListView_GetItemCount(listView);

	for (int i = 0; i < numItems; i++)
	{
		bool containsText = DoesItemRowContainText(listView, i, text, stringComparator);

		if (containsText)
		{
			return true;
		}
	}

	return false;
}

}

namespace ListViewHelper
{

void SelectItem(HWND hListView, int iItem, BOOL bSelect)
{
	UINT uNewState;

	if (bSelect)
	{
		uNewState = LVIS_SELECTED;
	}
	else
	{
		uNewState = 0;
	}

	ListView_SetItemState(hListView, iItem, uNewState, LVIS_SELECTED);
}

void SelectAllItems(HWND hListView, BOOL bSelect)
{
	UINT uNewState;

	if (bSelect)
	{
		uNewState = LVIS_SELECTED;
	}
	else
	{
		uNewState = 0;
	}

	SendMessage(hListView, WM_SETREDRAW, FALSE, 0);
	ListView_SetItemState(hListView, -1, uNewState, LVIS_SELECTED);
	SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
}

int InvertSelection(HWND hListView)
{
	int nTotalItems = ListView_GetItemCount(hListView);

	int nSelected = 0;

	SendMessage(hListView, WM_SETREDRAW, FALSE, 0);

	for (int i = 0; i < nTotalItems; i++)
	{
		if (ListView_GetItemState(hListView, i, LVIS_SELECTED) == LVIS_SELECTED)
		{
			SelectItem(hListView, i, FALSE);
		}
		else
		{
			SelectItem(hListView, i, TRUE);
			nSelected++;
		}
	}

	SendMessage(hListView, WM_SETREDRAW, TRUE, 0);

	return nSelected;
}

void FocusItem(HWND hListView, int iItem, BOOL bFocus)
{
	UINT uNewState;

	if (bFocus)
	{
		uNewState = LVIS_FOCUSED;
	}
	else
	{
		uNewState = 0;
	}

	ListView_SetItemState(hListView, iItem, uNewState, LVIS_FOCUSED);
}

void SetGridlines(HWND hListView, BOOL bEnableGridlines)
{
	auto dwExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	if (bEnableGridlines)
	{
		if ((dwExtendedStyle & LVS_EX_GRIDLINES) != LVS_EX_GRIDLINES)
		{
			dwExtendedStyle |= LVS_EX_GRIDLINES;
		}
	}
	else
	{
		if ((dwExtendedStyle & LVS_EX_GRIDLINES) == LVS_EX_GRIDLINES)
		{
			dwExtendedStyle &= ~LVS_EX_GRIDLINES;
		}
	}

	ListView_SetExtendedListViewStyle(hListView, dwExtendedStyle);
}

BOOL SetAutoArrange(HWND hListView, BOOL bAutoArrange)
{
	LONG_PTR lStyle = GetWindowLongPtr(hListView, GWL_STYLE);

	if (lStyle == 0)
	{
		return FALSE;
	}

	if (bAutoArrange)
	{
		if ((lStyle & LVS_AUTOARRANGE) != LVS_AUTOARRANGE)
		{
			lStyle |= LVS_AUTOARRANGE;
		}
	}
	else
	{
		if ((lStyle & LVS_AUTOARRANGE) == LVS_AUTOARRANGE)
		{
			lStyle &= ~LVS_AUTOARRANGE;
		}
	}

	SetLastError(0);
	LONG_PTR lRet = SetWindowLongPtr(hListView, GWL_STYLE, lStyle);

	if (lRet == 0 && GetLastError() != 0)
	{
		return FALSE;
	}

	return TRUE;
}

void ActivateOneClickSelect(HWND hListView, BOOL bActivate, UINT uHoverTime)
{
	auto dwExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	/* The three styles below are used to control one-click
	selection. */
	if (bActivate)
	{
		if ((dwExtendedStyle & LVS_EX_TRACKSELECT) != LVS_EX_TRACKSELECT)
		{
			dwExtendedStyle |= LVS_EX_TRACKSELECT;
		}

		if ((dwExtendedStyle & LVS_EX_ONECLICKACTIVATE) != LVS_EX_ONECLICKACTIVATE)
		{
			dwExtendedStyle |= LVS_EX_ONECLICKACTIVATE;
		}

		if ((dwExtendedStyle & LVS_EX_UNDERLINEHOT) != LVS_EX_UNDERLINEHOT)
		{
			dwExtendedStyle |= LVS_EX_UNDERLINEHOT;
		}

		ListView_SetExtendedListViewStyle(hListView, dwExtendedStyle);
		ListView_SetHoverTime(hListView, uHoverTime);
	}
	else
	{
		if ((dwExtendedStyle & LVS_EX_TRACKSELECT) == LVS_EX_TRACKSELECT)
		{
			dwExtendedStyle &= ~LVS_EX_TRACKSELECT;
		}

		if ((dwExtendedStyle & LVS_EX_ONECLICKACTIVATE) == LVS_EX_ONECLICKACTIVATE)
		{
			dwExtendedStyle &= ~LVS_EX_ONECLICKACTIVATE;
		}

		if ((dwExtendedStyle & LVS_EX_UNDERLINEHOT) == LVS_EX_UNDERLINEHOT)
		{
			dwExtendedStyle &= ~LVS_EX_UNDERLINEHOT;
		}

		ListView_SetExtendedListViewStyle(hListView, dwExtendedStyle);
	}
}

void AddRemoveExtendedStyle(HWND hListView, DWORD dwStyle, BOOL bAdd)
{
	auto dwExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	if (bAdd)
	{
		if ((dwExtendedStyle & dwStyle) != dwStyle)
		{
			dwExtendedStyle |= dwStyle;
		}
	}
	else
	{
		if ((dwExtendedStyle & dwStyle) == dwStyle)
		{
			dwExtendedStyle &= ~dwStyle;
		}
	}

	ListView_SetExtendedListViewStyle(hListView, dwExtendedStyle);
}

BOOL SwapItems(HWND hListView, int iItem1, int iItem2, BOOL bSwapLPARAM)
{
	UINT mask = LVIF_IMAGE | LVIF_INDENT | LVIF_STATE | LVIF_TEXT;
	UINT stateMask = static_cast<UINT>(-1);

	if (bSwapLPARAM)
	{
		mask |= LVIF_PARAM;
	}

	LVITEM lvItem1;
	TCHAR szText1[512];
	BOOL bRet1 = GetListViewItem(hListView, &lvItem1, mask, stateMask, iItem1, 0, szText1,
		SIZEOF_ARRAY(szText1));

	LVITEM lvItem2;
	TCHAR szText2[512];
	BOOL bRet2 = GetListViewItem(hListView, &lvItem2, mask, stateMask, iItem2, 0, szText2,
		SIZEOF_ARRAY(szText2));

	if (!bRet1 || !bRet2)
	{
		return FALSE;
	}

	lvItem1.iItem = iItem2;
	ListView_SetItem(hListView, &lvItem1);

	lvItem2.iItem = iItem1;
	ListView_SetItem(hListView, &lvItem2);

	HWND hHeader = ListView_GetHeader(hListView);
	int nColumns = Header_GetItemCount(hHeader);

	for (int i = 1; i < nColumns; i++)
	{
		TCHAR szColumn1[512];
		ListView_GetItemText(hListView, iItem1, i, szColumn1, SIZEOF_ARRAY(szColumn1));

		TCHAR szColumn2[512];
		ListView_GetItemText(hListView, iItem2, i, szColumn2, SIZEOF_ARRAY(szColumn2));

		ListView_SetItemText(hListView, iItem1, i, szColumn2);
		ListView_SetItemText(hListView, iItem2, i, szColumn1);
	}

	return TRUE;
}

void PositionInsertMark(HWND hListView, const POINT *ppt)
{
	/* Remove the insertion mark. */
	if (ppt == nullptr)
	{
		LVINSERTMARK lvim;
		lvim.cbSize = sizeof(LVINSERTMARK);
		lvim.dwFlags = 0;
		lvim.iItem = -1;
		ListView_SetInsertMark(hListView, &lvim);

		return;
	}

	RECT itemRect;
	DWORD dwFlags = 0;
	int iNext;
	BOOL bRet;

	LV_HITTESTINFO item;
	item.pt = *ppt;

	int iItem = ListView_HitTest(hListView, &item);

	if (iItem != -1 && item.flags & LVHT_ONITEM)
	{
		bRet = ListView_GetItemRect(hListView, item.iItem, &itemRect, LVIR_BOUNDS);

		/* If the cursor is on the left side
		of this item, set the insertion before
		this item; if it is on the right side
		of this item, set the insertion mark
		after this item. */
		if (bRet && (ppt->x - itemRect.left) > ((itemRect.right - itemRect.left) / 2))
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
		lvfi.flags = LVFI_NEARESTXY;
		lvfi.pt = *ppt;
		lvfi.vkDirection = VK_UP;
		iNext = ListView_FindItem(hListView, -1, &lvfi);

		if (iNext == -1)
		{
			lvfi.flags = LVFI_NEARESTXY;
			lvfi.pt = *ppt;
			lvfi.vkDirection = VK_LEFT;
			iNext = ListView_FindItem(hListView, -1, &lvfi);
		}

		bRet = ListView_GetItemRect(hListView, iNext, &itemRect, LVIR_BOUNDS);

		/* This situation only occurs at the
		end of the row. Prior to this, it is
		always the item on the right side that
		is found, with the insertion mark been
		inserted before that item.
		Once the end of the row is reached, the
		item found will be on the left side of
		the cursor. */
		if (bRet && ppt->x > (itemRect.left + ((itemRect.right - itemRect.left) / 2)))
		{
			/* At the end of a row, VK_UP appears to
			find the next item up. Therefore, if we're
			at the end of a row, and the cursor is
			completely below the "next" item, find the
			one under it instead (if there is an item
			under it), and anchor the insertion mark
			there. */
			if (ppt->y > itemRect.bottom)
			{
				int iBelow;

				iBelow = ListView_GetNextItem(hListView, iNext, LVNI_BELOW);

				if (iBelow != -1)
				{
					iNext = iBelow;
				}
			}

			dwFlags = LVIM_AFTER;
		}

		int nItems = ListView_GetItemCount(hListView);

		/* Last item is at position nItems - 1. */
		bRet = ListView_GetItemRect(hListView, nItems - 1, &itemRect, LVIR_BOUNDS);

		/* Special case needed for very last item. If cursor is within 0.5 to 1.5 width
		of last item, and is greater than it's y coordinate, snap the insertion mark to
		this item. */
		if (bRet && ppt->x > (itemRect.left + ((itemRect.right - itemRect.left) / 2))
			&& ppt->x < (itemRect.right + ((itemRect.right - itemRect.left) / 2) + 2)
			&& ppt->y > itemRect.top)
		{
			iNext = nItems - 1;
			dwFlags = LVIM_AFTER;
		}
	}

	LVINSERTMARK lvim;
	lvim.cbSize = sizeof(LVINSERTMARK);
	lvim.dwFlags = dwFlags;
	lvim.iItem = iNext;
	ListView_SetInsertMark(hListView, &lvim);
}

std::optional<int> GetLastSelectedItemIndex(HWND listView)
{
	int index = -1;
	int lastItemIndex = -1;

	// While the documentation for LVM_GETNEXTITEM seems to indicate that LVNI_PREVIOUS could be
	// used to find the last selected item (by reversing the direction of the search), that doesn't
	// appear to actually work (i.e. using LVNI_PREVIOUS | LVNI_SELECTED doesn't return any
	// results).
	// Which is why the last selected item is found by searching forwards instead.
	while ((index = ListView_GetNextItem(listView, index, LVNI_SELECTED)) != -1)
	{
		lastItemIndex = index;
	}

	if (lastItemIndex == -1)
	{
		return std::nullopt;
	}

	return lastItemIndex;
}

std::wstring GetItemText(HWND listView, int item, int subItem)
{
	std::wstring text;
	text.resize(260);

	while (true)
	{
		LVITEM lvItem = {};
		lvItem.iSubItem = subItem;
		lvItem.pszText = text.data();
		lvItem.cchTextMax = static_cast<int>(text.size());
		auto length = static_cast<int>(
			SendMessage(listView, LVM_GETITEMTEXT, item, reinterpret_cast<LPARAM>(&lvItem)));

		if (static_cast<size_t>(length) < (text.size() - 1))
		{
			text.resize(length);
			break;
		}
	}

	return text;
}

bool DoesListViewContainText(HWND listView, const std::wstring &text,
	StringComparatorFunc stringComparator)
{
	if (DoesHeaderContainText(listView, text, stringComparator))
	{
		return true;
	}

	if (DoAnyItemsContainText(listView, text, stringComparator))
	{
		return true;
	}

	return false;
}

}
