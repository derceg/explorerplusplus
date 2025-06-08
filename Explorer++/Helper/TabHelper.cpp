// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabHelper.h"
#include <wil/common.h>

namespace TabHelper
{

void SwapItems(HWND tabControl, int item1, int item2)
{
	TCITEM tcItem1 = {};
	tcItem1.mask = TCIF_PARAM | TCIF_IMAGE;
	auto res = TabCtrl_GetItem(tabControl, item1, &tcItem1);

	if (!res)
	{
		DCHECK(false);
		return;
	}

	auto text1 = GetItemText(tabControl, item1);

	TCITEM tcItem2 = {};
	tcItem2.mask = TCIF_PARAM | TCIF_IMAGE;
	res = TabCtrl_GetItem(tabControl, item2, &tcItem2);

	if (!res)
	{
		DCHECK(false);
		return;
	}

	auto text2 = GetItemText(tabControl, item2);

	WI_SetFlag(tcItem1.mask, TCIF_TEXT);
	tcItem1.pszText = const_cast<wchar_t *>(text1.c_str());
	res = TabCtrl_SetItem(tabControl, item2, &tcItem1);
	DCHECK(res);

	WI_SetFlag(tcItem2.mask, TCIF_TEXT);
	tcItem2.pszText = const_cast<wchar_t *>(text2.c_str());
	res = TabCtrl_SetItem(tabControl, item1, &tcItem2);
	DCHECK(res);

	int selectedIndex = TabCtrl_GetCurSel(tabControl);
	int updatedSelectionIndex = selectedIndex;

	if (selectedIndex == item1)
	{
		updatedSelectionIndex = item2;
	}
	else if (selectedIndex == item2)
	{
		updatedSelectionIndex = item1;
	}

	if (updatedSelectionIndex != selectedIndex)
	{
		int selectionRes = TabCtrl_SetCurSel(tabControl, updatedSelectionIndex);
		DCHECK_NE(selectionRes, -1);
	}
}

int MoveItem(HWND tabControl, int currentIndex, int newIndex)
{
	if (currentIndex == newIndex)
	{
		return currentIndex;
	}

	TCITEM tcItem = {};
	tcItem.mask = TCIF_PARAM | TCIF_IMAGE;
	BOOL res = TabCtrl_GetItem(tabControl, currentIndex, &tcItem);

	if (!res)
	{
		DCHECK(false);
		return currentIndex;
	}

	auto text = GetItemText(tabControl, currentIndex);
	WI_SetFlag(tcItem.mask, TCIF_TEXT);
	tcItem.pszText = const_cast<wchar_t *>(text.c_str());

	res = TabCtrl_DeleteItem(tabControl, currentIndex);

	if (!res)
	{
		DCHECK(false);
		return currentIndex;
	}

	int insertedIndex = TabCtrl_InsertItem(tabControl, newIndex, &tcItem);

	if (insertedIndex == -1)
	{
		DCHECK(false);
		return currentIndex;
	}

	return insertedIndex;
}

void SetItemText(HWND tabControl, int item, const std::wstring &text)
{
	TCITEM tcItem = {};
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = const_cast<wchar_t *>(text.c_str());
	auto res = TabCtrl_SetItem(tabControl, item, &tcItem);
	DCHECK(res);
}

std::wstring GetItemText(HWND tabControl, int item)
{
	std::wstring text;
	text.resize(260);

	while (true)
	{
		TCITEM tcItem = {};
		tcItem.mask = TCIF_TEXT;
		tcItem.pszText = text.data();
		tcItem.cchTextMax = static_cast<int>(text.size());
		auto res = TabCtrl_GetItem(tabControl, item, &tcItem);

		if (!res)
		{
			DCHECK(false);
			return L"";
		}

		if (!tcItem.pszText)
		{
			// The item has no text.
			return L"";
		}

		auto length = lstrlen(text.c_str());

		if (static_cast<size_t>(length) < (text.size() - 1))
		{
			text.resize(length);
			break;
		}

		text.resize(text.size() * 2);
	}

	return text;
}

}
