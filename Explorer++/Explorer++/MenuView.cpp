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

void MenuView::SetDelegate(MenuDelegate *delegate)
{
	m_delegate = delegate;
}

// If no delegate is set, a search for a delegate will proceed up the tree. This allows a menu with
// multiple submenus to be managed by a single delegate, while also allowing a submenu to be managed
// independently.
MenuDelegate *MenuView::MaybeGetDelegate()
{
	if (m_delegate)
	{
		return m_delegate;
	}

	if (m_parent)
	{
		return m_parent->MaybeGetDelegate();
	}

	return nullptr;
}

void MenuView::AppendItem(UINT id, const std::wstring &text,
	std::unique_ptr<const IconModel> iconModel, const std::wstring &helpText,
	const std::optional<std::wstring> &acceleratorText)
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
	auto res = InsertMenuItem(m_menu, GetMenuItemCount(m_menu), true, &menuItemInfo);
	CHECK(res);

	auto [itr, didInsert] = m_idToItemMap.try_emplace(id, std::move(iconModel), helpText);
	CHECK(didInsert);

	// It's only possible to add images to the menu when the DPI is known (so that the appropriate
	// DPI scaling factor can be applied). If there is no current DPI set (because the menu isn't
	// being shown), nothing needs to be done. The image will be added to the menu once the menu is
	// shown.
	if (m_currentDpi)
	{
		SetItemImage(id);
	}
}

MenuView *MenuView::AppendSubMenu(UINT id, const std::wstring &text,
	std::unique_ptr<const IconModel> iconModel)
{
	auto ownedSubMenu = MenuHelper::CheckedCreatePopupMenu();
	auto subMenu = ownedSubMenu.get();
	MenuHelper::AddSubMenuItem(m_menu, id, text, std::move(ownedSubMenu));

	auto [itr, didInsert] = m_idToItemMap.try_emplace(id, std::move(iconModel), L"");
	CHECK(didInsert);

	if (m_currentDpi)
	{
		SetItemImage(id);
	}

	auto ownedView = std::make_unique<MenuView>(subMenu);
	auto *view = ownedView.get();
	view->m_parent = this;

	m_subMenus.push_back(std::move(ownedView));

	return view;
}

void MenuView::SetItemImage(UINT id)
{
	const auto *item = GetItem(id);

	if (!item->iconModel)
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

void MenuView::AppendSeparator()
{
	MenuHelper::AddSeparator(m_menu);
}

void MenuView::EnableItem(UINT id, bool enable)
{
	MenuHelper::EnableItem(m_menu, id, enable);
}

void MenuView::CheckItem(UINT id, bool check)
{
	MenuHelper::CheckItem(m_menu, id, check);
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

	m_subMenus.clear();
	m_idToItemMap.clear();
	m_lastRenderedImageDpi.reset();
	m_weakPtrFactory.InvalidateWeakPtrs();
}

bool MenuView::IsItemEnabled(UINT id) const
{
	return MenuHelper::IsMenuItemEnabled(m_menu, id, false);
}

int MenuView::GetNumItems() const
{
	int numItems = GetMenuItemCount(m_menu);
	CHECK(numItems != -1);
	return numItems;
}

void MenuView::OnPopupWillShowForDpi(UINT dpi)
{
	m_currentDpi = dpi;

	MaybeAddImagesToMenu();
}

void MenuView::OnPopupClosed()
{
	m_currentDpi.reset();
}

MenuView *MenuView::MaybeGetMenuViewForNativeMenu(HMENU menu)
{
	if (m_menu == menu)
	{
		return this;
	}

	for (auto &submenu : m_subMenus)
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
	if (m_idToItemMap.contains(id))
	{
		return this;
	}

	for (auto &subMenu : m_subMenus)
	{
		if (auto *view = subMenu->MaybeGetMenuViewForItem(id))
		{
			return view;
		}
	}

	return nullptr;
}

void MenuView::MaybeAddImagesToMenu()
{
	if (GetCurrentDpi() == m_lastRenderedImageDpi)
	{
		// The DPI hasn't changed since the images were last added, so there's nothing that needs to
		// be done.
		return;
	}

	// If the DPI has changed, any previous image requests can be ignored.
	m_weakPtrFactory.InvalidateWeakPtrs();

	for (UINT id : m_idToItemMap | std::views::keys)
	{
		SetItemImage(id);
	}

	m_lastRenderedImageDpi = GetCurrentDpi();
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

UINT MenuView::GetItemId(int index) const
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_ID;
	auto res = GetMenuItemInfo(m_menu, index, true, &menuItemInfo);
	CHECK(res);

	return menuItemInfo.wID;
}

std::optional<UINT> MenuView::MaybeGetItemAtPoint(const POINT &ptScreen) const
{
	return MenuHelper::MaybeGetMenuItemAtPoint(m_menu, ptScreen);
}

HMENU MenuView::GetNativeMenuForTesting() const
{
	CHECK(IsInTest());

	return m_menu;
}

UINT MenuView::GetItemIdForTesting(int index) const
{
	CHECK(IsInTest());

	return GetItemId(index);
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
