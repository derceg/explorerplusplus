// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <optional>

namespace MenuHelper
{

enum class ItemLookup
{
	ById,
	ByPosition
};

void AddStyleFlags(HMENU menu, DWORD styleFlags);
DWORD GetStyleFlags(HMENU menu);

void AddStringItem(HMENU menu, UINT id, const std::wstring &text);
void AddStringItem(HMENU menu, UINT id, const std::wstring &text, UINT item, ItemLookup lookupType);
void AddSeparator(HMENU menu);
void AddSeparator(HMENU menu, UINT item, ItemLookup lookupType);
HMENU AddNewSubMenuItem(HMENU menu, UINT id, const std::wstring &text);
HMENU AddNewSubMenuItem(HMENU menu, UINT id, const std::wstring &text, UINT item,
	ItemLookup lookupType);

void AddItemTypeFlags(HMENU menu, UINT item, ItemLookup lookupType, UINT typeFlags);
void SetItemChecked(HMENU menu, UINT item, ItemLookup lookupType, bool check);
void SetItemEnabled(HMENU menu, UINT item, ItemLookup lookupType, bool enable);

void RemoveDuplicateSeparators(HMENU menu);
void RemoveTrailingSeparators(HMENU menu);

std::wstring GetItemText(HMENU menu, UINT item, ItemLookup lookupType);
std::optional<UINT> MaybeGetItemIdIncludingSubmenu(HMENU menu, int index);
bool IsPartOfMenu(HMENU menu, HMENU potentiallyRelatedMenu);
HBITMAP GetItemBitmap(HMENU menu, UINT item, ItemLookup lookupType);
bool IsItemEnabled(HMENU menu, UINT item, ItemLookup lookupType);
bool IsItemChecked(HMENU menu, UINT item, ItemLookup lookupType);
UINT GetItemState(HMENU menu, UINT item, ItemLookup lookupType);
UINT GetItemTypeFlags(HMENU menu, UINT item, ItemLookup lookupType);

std::optional<UINT> MaybeGetHighlightedItem(HMENU menu);

// Returns the ID of the menu item at the specified point in screen coordinates, if there is such an
// item. Only items that are part of the provided menu will be considered. Note that a value will
// only be returned if the menu is the top-most menu. If another menu is shown on top, no result
// will be returned, regardless of the location provided.
std::optional<UINT> MaybeGetItemAtPoint(HMENU menu, const POINT &ptScreen);

wil::unique_hmenu CheckedCreateMenu();
wil::unique_hmenu CheckedCreatePopupMenu();

}
