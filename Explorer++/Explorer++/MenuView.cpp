// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuView.h"
#include "../Helper/MenuHelper.h"

MenuView::MenuView() : m_destroyed(std::make_shared<bool>(false))
{
}

MenuView::~MenuView()
{
	m_viewDestroyedSignal();

	*m_destroyed = true;
}

void MenuView::AppendItem(UINT id, const std::wstring &text, const ShellIconModel &shellIcon,
	const std::wstring &helpText, const std::optional<std::wstring> &acceleratorText)
{
	// The value 0 shouldn't be used as an item ID. That's because a call like TrackPopupMenu() will
	// use a return value of 0 to indicate the menu was canceled, or an error occurred.
	DCHECK_NE(id, 0U);

	std::wstring finalText = text;

	if (acceleratorText)
	{
		finalText += L"\t" + *acceleratorText;
	}

	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_ID | MIIM_STRING;
	menuItemInfo.wID = id;
	menuItemInfo.dwTypeData = finalText.data();

	int iconCallbackId = m_iconCallbackIdCounter++;

	auto bitmap = shellIcon.GetBitmap(ShellIconSize::Small,
		[this, id, iconCallbackId, destroyed = m_destroyed](wil::unique_hbitmap updatedIcon)
		{
			if (*destroyed)
			{
				// The icon can be returned after the menu has been closed. In that case, there's
				// nothing that needs to be done.
				return;
			}

			OnUpdatedIconRetrieved(id, iconCallbackId, std::move(updatedIcon));
		});

	auto [itrIconCallback, didInsertIconCallback] = m_pendingIconCallbackIds.insert(iconCallbackId);
	DCHECK(didInsertIconCallback);

	if (bitmap)
	{
		menuItemInfo.fMask |= MIIM_BITMAP;
		menuItemInfo.hbmpItem = bitmap.get();

		// The CHECK here is present because the bitmap needs to be stored while the menu exists.
		// There shouldn't be items with duplicate IDs, so this insert should always succeed.
		auto [itr, didInsert] = m_itemImageMapping.insert({ id, std::move(bitmap) });
		CHECK(didInsert);
	}

	auto res = InsertMenuItem(GetMenu(), GetMenuItemCount(GetMenu()), true, &menuItemInfo);
	CHECK(res);

	auto [itr, didInsert] = m_itemHelpTextMapping.insert({ id, helpText });
	DCHECK(didInsert);
}

void MenuView::OnUpdatedIconRetrieved(UINT id, int iconCallbackId, wil::unique_hbitmap updatedIcon)
{
	auto itr = m_pendingIconCallbackIds.find(iconCallbackId);

	// As the menu can be cleared when the set of items changes, this icon notification might be for
	// a previous menu item. If it is, it can be ignored.
	if (itr == m_pendingIconCallbackIds.end())
	{
		return;
	}

	m_pendingIconCallbackIds.erase(itr);

	UpdateBitmapForItem(id, std::move(updatedIcon));
}

void MenuView::UpdateBitmapForItem(UINT id, wil::unique_hbitmap bitmap)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	menuItemInfo.hbmpItem = bitmap.get();
	auto res = SetMenuItemInfo(GetMenu(), id, false, &menuItemInfo);
	CHECK(res);

	if (bitmap)
	{
		m_itemImageMapping.insert_or_assign(id, std::move(bitmap));
	}
}

void MenuView::AppendSeparator()
{
	MenuHelper::AddSeparator(GetMenu());
}

void MenuView::EnableItem(UINT id, bool enable)
{
	MenuHelper::EnableItem(GetMenu(), id, enable);
}

void MenuView::ClearMenu()
{
	for (int i = GetMenuItemCount(GetMenu()) - 1; i >= 0; i--)
	{
		auto res = DeleteMenu(GetMenu(), i, MF_BYPOSITION);
		DCHECK(res);
	}

	m_itemImageMapping.clear();
	m_pendingIconCallbackIds.clear();
	m_itemHelpTextMapping.clear();
}

std::wstring MenuView::GetHelpTextForItem(UINT id) const
{
	auto itr = m_itemHelpTextMapping.find(id);
	CHECK(itr != m_itemHelpTextMapping.end());
	return itr->second;
}

void MenuView::SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (!MenuHelper::IsMenuItemEnabled(GetMenu(), id, false))
	{
		return;
	}

	m_itemSelectedSignal(id, isCtrlKeyDown, isShiftKeyDown);
}

void MenuView::MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (!MenuHelper::IsMenuItemEnabled(GetMenu(), id, false))
	{
		return;
	}

	m_itemMiddleClickedSignal(id, isCtrlKeyDown, isShiftKeyDown);
}

boost::signals2::connection MenuView::AddItemSelectedObserver(
	const ItemSelectedSignal::slot_type &observer)
{
	return m_itemSelectedSignal.connect(observer);
}

boost::signals2::connection MenuView::AddItemMiddleClickedObserver(
	const ItemMiddleClickedSignal::slot_type &observer)
{
	return m_itemMiddleClickedSignal.connect(observer);
}

boost::signals2::connection MenuView::AddViewDestroyedObserver(
	const ViewDestroyedSignal::slot_type &observer)
{
	return m_viewDestroyedSignal.connect(observer);
}

int MenuView::GetItemCountForTesting() const
{
	return GetMenuItemCount(GetMenu());
}

UINT MenuView::GetItemIdForTesting(int index) const
{
	return GetMenuItemID(GetMenu(), index);
}

std::wstring MenuView::GetItemTextForTesting(UINT id) const
{
	TCHAR text[256];

	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STRING;
	menuItemInfo.dwTypeData = text;
	menuItemInfo.cch = static_cast<UINT>(std::size(text));
	auto res = GetMenuItemInfo(GetMenu(), id, false, &menuItemInfo);
	CHECK(res);

	return text;
}

HBITMAP MenuView::GetItemBitmapForTesting(UINT id) const
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	auto res = GetMenuItemInfo(GetMenu(), id, false, &menuItemInfo);
	CHECK(res);

	return menuItemInfo.hbmpItem;
}
