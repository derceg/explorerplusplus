// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ListViewHelper.h"
#include "ScopedRedrawDisabler.h"
#include "WindowHelper.h"
#include <wil/common.h>

namespace
{

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

void SelectItem(HWND listView, int item, bool select)
{
	UINT updatedState = select ? LVIS_SELECTED : 0;
	ListView_SetItemState(listView, item, updatedState, LVIS_SELECTED);
}

void SelectAllItems(HWND listView, bool select)
{
	ScopedRedrawDisabler redrawDisabler(listView);
	UINT updatedState = select ? LVIS_SELECTED : 0;
	ListView_SetItemState(listView, -1, updatedState, LVIS_SELECTED);
}

void InvertSelection(HWND listView)
{
	int numItems = ListView_GetItemCount(listView);

	ScopedRedrawDisabler redrawDisabler(listView);

	for (int i = 0; i < numItems; i++)
	{
		if (ListView_GetItemState(listView, i, LVIS_SELECTED) == LVIS_SELECTED)
		{
			SelectItem(listView, i, false);
		}
		else
		{
			SelectItem(listView, i, true);
		}
	}
}

void FocusItem(HWND listView, int item, bool focus)
{
	UINT updatedState = focus ? LVIS_FOCUSED : 0;
	ListView_SetItemState(listView, item, updatedState, LVIS_FOCUSED);
}

void SetAutoArrange(HWND listView, bool autoArrange)
{
	AddWindowStyles(listView, LVS_AUTOARRANGE, autoArrange);
}

void ActivateOneClickSelect(HWND listView, bool activate, UINT hoverTime)
{
	AddRemoveExtendedStyles(listView,
		LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_UNDERLINEHOT, activate);

	if (activate)
	{
		ListView_SetHoverTime(listView, hoverTime);
	}
}

void AddRemoveExtendedStyles(HWND listView, DWORD styles, bool add)
{
	auto extendedStyle = ListView_GetExtendedListViewStyle(listView);

	if (add)
	{
		WI_SetAllFlags(extendedStyle, styles);
	}
	else
	{
		WI_ClearAllFlags(extendedStyle, styles);
	}

	ListView_SetExtendedListViewStyle(listView, extendedStyle);
}

void SwapItems(HWND listView, int item1, int item2)
{
	UINT mask = LVIF_IMAGE | LVIF_INDENT | LVIF_STATE | LVIF_PARAM;
	auto stateMask = static_cast<UINT>(-1);

	LVITEM lvItem1 = {};
	lvItem1.mask = mask;
	lvItem1.stateMask = stateMask;
	lvItem1.iItem = item1;
	lvItem1.iSubItem = 0;
	auto res = ListView_GetItem(listView, &lvItem1);

	if (!res)
	{
		DCHECK(false);
		return;
	}

	LVITEM lvItem2 = {};
	lvItem2.mask = mask;
	lvItem2.stateMask = stateMask;
	lvItem2.iItem = item2;
	lvItem2.iSubItem = 0;
	res = ListView_GetItem(listView, &lvItem2);

	if (!res)
	{
		DCHECK(false);
		return;
	}

	ScopedRedrawDisabler redrawDisabler(listView);

	lvItem1.iItem = item2;
	res = ListView_SetItem(listView, &lvItem1);
	DCHECK(res);

	lvItem2.iItem = item1;
	res = ListView_SetItem(listView, &lvItem2);
	DCHECK(res);

	HWND header = ListView_GetHeader(listView);
	int numColumns = Header_GetItemCount(header);

	for (int i = 0; i < numColumns; i++)
	{
		auto item1ColumnText = GetItemText(listView, item1, i);
		auto item2ColumnText = GetItemText(listView, item2, i);

		ListView_SetItemText(listView, item1, i, item2ColumnText.data());
		ListView_SetItemText(listView, item2, i, item1ColumnText.data());
	}
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

		text.resize(text.size() * 2);
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

std::set<int> GetSelectedItems(HWND listView)
{
	std::set<int> selectedItems;
	int index = -1;

	while ((index = ListView_GetNextItem(listView, index, LVNI_SELECTED)) != -1)
	{
		selectedItems.insert(index);
	}

	return selectedItems;
}

}
