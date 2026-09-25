// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuView.h"
#include "TestHelper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WeakPtr.h"
#include <ranges>

MenuView::MenuView(HMENU menu) : m_menu(menu)
{
}

bool MenuView::IsRoot() const
{
	return m_parent == nullptr;
}

void MenuView::EnableDragAndDrop(bool enable)
{
	MENUINFO menuInfo = {};
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_STYLE;
	auto res = GetMenuInfo(m_menu, &menuInfo);
	CHECK(res);

	if (enable)
	{
		WI_SetFlag(menuInfo.dwStyle, MNS_DRAGDROP);
	}
	else
	{
		WI_ClearFlag(menuInfo.dwStyle, MNS_DRAGDROP);
	}

	res = SetMenuInfo(m_menu, &menuInfo);
	CHECK(res);
}

void MenuView::AppendItem(MenuDelegate *delegate, UINT id, const std::wstring &text,
	std::unique_ptr<const IconModel> iconModel, const std::wstring &helpText,
	const std::optional<std::wstring> &acceleratorText)
{
	AppendItem(delegate, id, text, std::move(iconModel), helpText, acceleratorText,
		CheckStyle::Normal);
}

void MenuView::AppendRadioItem(MenuDelegate *delegate, UINT id, const std::wstring &text,
	std::unique_ptr<const IconModel> iconModel, const std::wstring &helpText,
	const std::optional<std::wstring> &acceleratorText)
{
	AppendItem(delegate, id, text, std::move(iconModel), helpText, acceleratorText,
		CheckStyle::Radio);
}

// Note that MenuDelegate is effectively a menu-level class, but it can be set with item-level
// granularity. That allows different items on the same menu to be managed by different delegates.
void MenuView::AppendItem(MenuDelegate *delegate, UINT id, const std::wstring &text,
	std::unique_ptr<const IconModel> iconModel, const std::wstring &helpText,
	const std::optional<std::wstring> &acceleratorText, CheckStyle checkStyle)
{
	// The value 0 shouldn't be used as an item ID. That's because a call like TrackPopupMenu() will
	// use a return value of 0 to indicate the menu was canceled, or an error occurred.
	CHECK_NE(id, 0U);

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

	if (checkStyle == CheckStyle::Radio)
	{
		WI_SetFlag(menuItemInfo.fMask, MIIM_FTYPE);
		menuItemInfo.fType = MFT_RADIOCHECK;
	}

	auto res = InsertMenuItem(m_menu, GetMenuItemCount(m_menu), true, &menuItemInfo);
	CHECK(res);

	auto [itr, didInsert] = m_idToItemMap.try_emplace(id, delegate, std::move(iconModel), helpText);
	CHECK(didInsert);

	if (m_isShowing)
	{
		UpdateDynamicPropertiesForItem(id);
	}
}

MenuView *MenuView::AppendSubMenu(MenuDelegate *delegate, UINT id, const std::wstring &text,
	std::unique_ptr<const IconModel> iconModel)
{
	auto ownedSubMenu = MenuHelper::CheckedCreatePopupMenu();
	auto subMenu = ownedSubMenu.get();
	MenuHelper::AddSubMenuItem(m_menu, id, text, std::move(ownedSubMenu));

	auto [itr, didInsert] = m_idToItemMap.try_emplace(id, delegate, std::move(iconModel), L"");
	CHECK(didInsert);

	auto ownedView = std::make_unique<MenuView>(subMenu);
	auto *view = ownedView.get();
	view->m_parent = this;

	m_idToSubMenuMap.insert({ id, std::move(ownedView) });

	if (m_isShowing)
	{
		UpdateDynamicPropertiesForItem(id);
	}

	return view;
}

void MenuView::AppendSeparator()
{
	MenuHelper::AddSeparator(m_menu);
}

void MenuView::RemoveDuplicateSeparators()
{
	MenuHelper::RemoveDuplicateSeparators(m_menu);
}

void MenuView::RemoveTrailingSeparators()
{
	MenuHelper::RemoveTrailingSeparators(m_menu);
}

void MenuView::ClearMenu()
{
	for (int i = GetMenuItemCount(m_menu) - 1; i >= 0; i--)
	{
		auto res = DeleteMenu(m_menu, i, MF_BYPOSITION);
		CHECK(res);
	}

	m_idToSubMenuMap.clear();
	m_idToItemMap.clear();
	m_weakPtrFactory.InvalidateWeakPtrs();
}

int MenuView::GetNumItems() const
{
	int numItems = GetMenuItemCount(m_menu);
	CHECK(numItems != -1);
	return numItems;
}

void MenuView::OnPopupWillShowForDpi(UINT dpi)
{
	if (m_currentDpi && dpi != m_currentDpi)
	{
		// The DPI has changed, so any previous image requests can be ignored.
		m_weakPtrFactory.InvalidateWeakPtrs();

		ClearItemImages();
	}

	m_currentDpi = dpi;
	m_isShowing = true;

	UpdateDynamicPropertiesForAllItems();
}

void MenuView::OnPopupClosed()
{
	m_isShowing = false;
}

MenuView *MenuView::MaybeGetMenuViewForNativeMenu(HMENU menu)
{
	if (m_menu == menu)
	{
		return this;
	}

	for (auto &submenu : m_idToSubMenuMap | std::views::values)
	{
		if (auto *view = submenu->MaybeGetMenuViewForNativeMenu(menu))
		{
			return view;
		}
	}

	return nullptr;
}

MenuView *MenuView::MaybeGetMenuViewForItem(UINT id)
{
	return const_cast<MenuView *>(std::as_const(*this).MaybeGetMenuViewForItem(id));
}

const MenuView *MenuView::MaybeGetMenuViewForItem(UINT id) const
{
	if (m_idToItemMap.contains(id))
	{
		return this;
	}

	for (auto &subMenu : m_idToSubMenuMap | std::views::values)
	{
		if (auto *view = subMenu->MaybeGetMenuViewForItem(id))
		{
			return view;
		}
	}

	return nullptr;
}

void MenuView::ClearItemImages()
{
	for (UINT id : m_idToItemMap | std::views::keys)
	{
		UpdateItemBitmap(id, nullptr);
	}
}

void MenuView::UpdateDynamicPropertiesForAllItems()
{
	for (UINT id : m_idToItemMap | std::views::keys)
	{
		UpdateDynamicPropertiesForItem(id);
	}
}

// Updates item properties that are only set when the menu is shown.
void MenuView::UpdateDynamicPropertiesForItem(UINT id)
{
	UpdateItemState(id);
	SetItemImage(id);
}

void MenuView::UpdateItemState(UINT id)
{
	auto *delegate = MaybeGetDelegateForItem(id);

	if (!delegate)
	{
		return;
	}

	MenuHelper::EnableItem(m_menu, id, delegate->IsItemEnabled(id));
	MenuHelper::CheckItem(m_menu, id, delegate->IsItemChecked(id));
}

void MenuView::SetItemImage(UINT id)
{
	const auto *item = GetItem(id);

	if (!item->iconModel || item->bitmap)
	{
		return;
	}

	auto bitmap = item->iconModel->GetBitmap(GetCurrentDpi(),
		[id, self = m_weakPtrFactory.GetWeakPtr()](wil::unique_hbitmap updatedBitmap)
		{
			if (!self)
			{
				// The updated image can be returned after the menu has been closed or cleared. In
				// either case, there's nothing that needs to be done.
				return;
			}

			self->UpdateItemBitmap(id, std::move(updatedBitmap));
		});

	UpdateItemBitmap(id, std::move(bitmap));
}

void MenuView::UpdateItemBitmap(UINT id, wil::unique_hbitmap bitmap)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	menuItemInfo.hbmpItem = bitmap.get();
	auto res = SetMenuItemInfo(m_menu, id, false, &menuItemInfo);
	CHECK(res);

	auto *item = GetItem(id);
	item->bitmap = std::move(bitmap);
}

UINT MenuView::GetCurrentDpi() const
{
	CHECK(m_currentDpi);
	return *m_currentDpi;
}

std::wstring MenuView::GetItemHelpText(UINT id) const
{
	const auto *item = GetItem(id);
	return item->helpText;
}

MenuView::Item *MenuView::GetItem(int id)
{
	auto itr = m_idToItemMap.find(id);
	CHECK(itr != m_idToItemMap.end());
	return &itr->second;
}

const MenuView::Item *MenuView::GetItem(int id) const
{
	auto itr = m_idToItemMap.find(id);
	CHECK(itr != m_idToItemMap.end());
	return &itr->second;
}

// This will only return IDs for non-separators.
std::optional<UINT> MenuView::MaybeGetItemId(int index) const
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_ID;
	auto res = GetMenuItemInfo(m_menu, index, true, &menuItemInfo);
	CHECK(res);

	if (menuItemInfo.wID == 0)
	{
		// This is a separator item.
		return std::nullopt;
	}

	return menuItemInfo.wID;
}

MenuDelegate *MenuView::MaybeGetDelegateForItem(UINT id)
{
	return const_cast<MenuDelegate *>(std::as_const(*this).MaybeGetDelegateForItem(id));
}

const MenuDelegate *MenuView::MaybeGetDelegateForItem(UINT id) const
{
	auto itr = m_idToItemMap.find(id);
	CHECK(itr != m_idToItemMap.end());
	return itr->second.delegate;
}

std::optional<UINT> MenuView::MaybeGetItemAtPoint(const POINT &ptScreen) const
{
	auto id = MenuHelper::MaybeGetMenuItemAtPoint(m_menu, ptScreen);

	if (!id || !MaybeGetMenuViewForItem(*id))
	{
		return std::nullopt;
	}

	return *id;
}

HMENU MenuView::GetNativeMenuForTesting() const
{
	CHECK(IsInTest());

	return m_menu;
}

UINT MenuView::GetItemIdForTesting(int index) const
{
	CHECK(IsInTest());

	auto id = MaybeGetItemId(index);
	CHECK(id);
	return *id;
}

MenuDelegate *MenuView::MaybeGetDelegateForItemForTesting(UINT id)
{
	CHECK(IsInTest());

	return MaybeGetDelegateForItem(id);
}

const MenuDelegate *MenuView::MaybeGetDelegateForItemForTesting(UINT id) const
{
	CHECK(IsInTest());

	return MaybeGetDelegateForItem(id);
}

std::wstring MenuView::GetItemTextForTesting(UINT id) const
{
	CHECK(IsInTest());

	wchar_t text[256];

	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STRING;
	menuItemInfo.dwTypeData = text;
	menuItemInfo.cch = std::size(text);
	auto res = GetMenuItemInfo(m_menu, id, false, &menuItemInfo);
	CHECK(res);

	return text;
}

HBITMAP MenuView::GetItemBitmapForTesting(UINT id) const
{
	CHECK(IsInTest());

	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	auto res = GetMenuItemInfo(m_menu, id, false, &menuItemInfo);
	CHECK(res);

	return menuItemInfo.hbmpItem;
}

const MenuView *MenuView::GetSubMenuViewForTesting(UINT id) const
{
	CHECK(IsInTest());

	auto itr = m_idToSubMenuMap.find(id);
	CHECK(itr != m_idToSubMenuMap.end());
	return itr->second.get();
}

void MenuView::OnPopupWillShowForTesting(UINT dpi)
{
	CHECK(IsInTest());

	OnPopupWillShowForDpi(dpi);
}

void MenuView::OnPopupClosedForTesting()
{
	CHECK(IsInTest());

	OnPopupClosed();
}
