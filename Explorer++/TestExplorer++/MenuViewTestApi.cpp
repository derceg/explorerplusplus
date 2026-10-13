// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MenuViewTestApi.h"
#include "MenuView.h"
#include "../Helper/MenuHelper.h"
#include <ranges>

HMENU MenuViewTestApi::GetNativeMenu(const MenuView *menuView)
{
	return menuView->m_menu;
}

UINT MenuViewTestApi::GetItemId(const MenuView *menuView, int index)
{
	auto id = MenuHelper::MaybeGetItemIdIncludingSubmenu(menuView->m_menu, index);
	CHECK(id);
	return *id;
}

std::optional<UINT> MenuViewTestApi::MaybeGetIdForKey(const MenuView *menuView,
	const MenuItemKey &key)
{
	auto itr = menuView->m_keyToIdMap.find(key);

	if (itr != menuView->m_keyToIdMap.end())
	{
		return itr->second;
	}

	for (auto &menuItem : menuView->m_idToItemMap | std::views::values)
	{
		auto *subMenuView = menuItem.GetSubMenuView();

		if (!subMenuView)
		{
			continue;
		}

		if (auto id = MaybeGetIdForKey(subMenuView, key))
		{
			return *id;
		}
	}

	return std::nullopt;
}

std::wstring MenuViewTestApi::GetItemFullText(const MenuView *menuView, UINT id)
{
	auto *item = menuView->GetItem(id);
	return item->GetFullText();
}

std::wstring MenuViewTestApi::GetItemHelpText(const MenuView *menuView, UINT id)
{
	auto *item = menuView->GetItem(id);
	return item->GetHelpText();
}

HBITMAP MenuViewTestApi::GetItemBitmap(const MenuView *menuView, UINT id)
{
	auto *item = menuView->GetItem(id);
	return item->GetBitmap();
}

bool MenuViewTestApi::IsItemEnabled(const MenuView *menuView, const MenuItemKey &key)
{
	auto id = MaybeGetIdForKey(menuView, key);
	CHECK(id);
	return IsItemEnabled(menuView, *id);
}

bool MenuViewTestApi::IsItemEnabled(const MenuView *menuView, UINT id)
{
	const auto *item = menuView->GetItem(id);
	return item->IsEnabled();
}

bool MenuViewTestApi::IsItemChecked(const MenuView *menuView, const MenuItemKey &key)
{
	auto id = MaybeGetIdForKey(menuView, key);
	CHECK(id);
	return IsItemChecked(menuView, *id);
}

bool MenuViewTestApi::IsItemChecked(const MenuView *menuView, UINT id)
{
	const auto *item = menuView->GetItem(id);
	return item->IsChecked();
}

const MenuView *MenuViewTestApi::GetSubMenuView(const MenuView *menuView, const MenuItemKey &key)
{
	auto id = MaybeGetIdForKey(menuView, key);
	CHECK(id);
	return GetSubMenuView(menuView, *id);
}

const MenuView *MenuViewTestApi::GetSubMenuView(const MenuView *menuView, UINT id)
{
	auto itr = menuView->m_idToItemMap.find(id);
	CHECK(itr != menuView->m_idToItemMap.end());

	auto *subMenuView = itr->second.GetSubMenuView();
	CHECK(subMenuView);

	return subMenuView;
}

wil::com_ptr_nothrow<IDropTarget> MenuViewTestApi::MaybeGetDropTarget(MenuView *menuView, UINT id,
	MenuDropPosition position)
{
	auto *item = menuView->GetItem(id);
	return item->MaybeGetDropTarget(position);
}

void MenuViewTestApi::OnPopupWillShowForDpi(MenuView *menuView, UINT dpi)
{
	menuView->OnPopupWillShowForDpi(dpi);
}

void MenuViewTestApi::OnPopupClosed(MenuView *menuView)
{
	menuView->OnPopupClosed();
}
