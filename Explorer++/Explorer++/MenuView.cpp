// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuView.h"
#include "TestHelper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WeakPtr.h"
#include <ranges>

MenuView::MenuView(HMENU menu) : m_parent(nullptr), m_menu(menu)
{
	// Not all menus support drag and drop, but for those that don't, the menu drag messages will
	// simply be ignored.
	MenuHelper::AddStyleFlags(m_menu, MNS_DRAGDROP);
}

MenuView::MenuView(MenuView *parent, HMENU menu, PassKey) : m_parent(parent), m_menu(menu)
{
}

bool MenuView::IsRoot() const
{
	return m_parent == nullptr;
}

void MenuView::AppendItem(MenuItemParams params)
{
	UINT id = AllocateId();
	Item item(params);

	MenuHelper::AddStringItem(m_menu, id, item.GetFullText());

	if (params.GetCheckStyle() == MenuItemCheckStyle::Radio)
	{
		MenuHelper::AddItemTypeFlags(m_menu, id, MFT_RADIOCHECK);
	}

	RegisterItem(id, std::move(item), params.GetKey());
}

MenuView *MenuView::AppendSubMenu(SubMenuItemParams params)
{
	UINT id = AllocateId();
	HMENU subMenu = MenuHelper::AddNewSubMenuItem(m_menu, id, params.GetText());

	auto *item = RegisterItem(id,
		Item(params, std::make_unique<MenuView>(this, subMenu, PassKey())), params.GetKey());

	return item->GetSubMenuView();
}

UINT MenuView::AllocateId()
{
	auto *root = GetRoot();
	return root->m_idCounter++;
}

MenuView *MenuView::GetRoot()
{
	auto *menuView = this;

	while (menuView->m_parent)
	{
		menuView = menuView->m_parent;
	}

	return menuView;
}

MenuView::Item *MenuView::RegisterItem(UINT id, Item item, const std::optional<MenuItemKey> &key)
{
	auto [itr, didInsert] = m_idToItemMap.try_emplace(id, std::move(item));
	CHECK(didInsert);

	if (key)
	{
		auto [keyItr, didInsertKey] = m_keyToIdMap.insert({ *key, id });
		CHECK(didInsertKey);
	}

	if (m_isShowing)
	{
		UpdateDynamicPropertiesForItem(id);
	}

	return &itr->second;
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

	m_idToItemMap.clear();
	m_keyToIdMap.clear();
	m_weakPtrFactory.InvalidateWeakPtrs();

	if (IsRoot())
	{
		m_idCounter = INITIAL_ID;
	}
}

boost::signals2::connection MenuView::AddPopupWillShowObserver(
	const PopupWillShowSignal::slot_type &observer)
{
	return m_popupWillShowSignal.connect(observer);
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

	m_popupWillShowSignal();

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

	for (auto &menuItem : m_idToItemMap | std::views::values)
	{
		auto *subMenuView = menuItem.GetSubMenuView();

		if (!subMenuView)
		{
			continue;
		}

		if (auto *view = subMenuView->MaybeGetMenuViewForNativeMenu(menu))
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

	for (auto &menuItem : m_idToItemMap | std::views::values)
	{
		auto *subMenuView = menuItem.GetSubMenuView();

		if (!subMenuView)
		{
			continue;
		}

		if (auto *view = subMenuView->MaybeGetMenuViewForItem(id))
		{
			return view;
		}
	}

	return nullptr;
}

std::optional<UINT> MenuView::MaybeGetIdForKey(const MenuItemKey &key) const
{
	auto itr = m_keyToIdMap.find(key);

	if (itr != m_keyToIdMap.end())
	{
		return itr->second;
	}

	for (auto &menuItem : m_idToItemMap | std::views::values)
	{
		auto *subMenuView = menuItem.GetSubMenuView();

		if (!subMenuView)
		{
			continue;
		}

		if (auto id = subMenuView->MaybeGetIdForKey(key))
		{
			return *id;
		}
	}

	return std::nullopt;
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
	auto *item = GetItem(id);
	MenuHelper::SetItemEnabled(m_menu, id, item->IsEnabled());
	MenuHelper::SetItemChecked(m_menu, id, item->IsChecked());
}

void MenuView::SetItemImage(UINT id)
{
	const auto *item = GetItem(id);

	if (!item->GetIconModel() || item->GetBitmap())
	{
		return;
	}

	auto bitmap = item->GetIconModel()->GetBitmap(GetCurrentDpi(),
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
	item->UpdateBitmap(std::move(bitmap));
}

UINT MenuView::GetCurrentDpi() const
{
	CHECK(m_currentDpi);
	return *m_currentDpi;
}

std::wstring MenuView::GetItemHelpText(UINT id) const
{
	const auto *item = GetItem(id);
	return item->GetHelpText();
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

std::optional<UINT> MenuView::MaybeGetItemAtPoint(const POINT &ptScreen) const
{
	auto id = MenuHelper::MaybeGetItemAtPoint(m_menu, ptScreen);

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

std::optional<UINT> MenuView::MaybeGetIdForKeyForTesting(const MenuItemKey &key) const
{
	CHECK(IsInTest());

	return MaybeGetIdForKey(key);
}

std::wstring MenuView::GetItemFullTextForTesting(UINT id) const
{
	CHECK(IsInTest());

	auto *item = GetItem(id);
	return item->GetFullText();
}

HBITMAP MenuView::GetItemBitmapForTesting(UINT id) const
{
	CHECK(IsInTest());

	auto *item = GetItem(id);
	return item->GetBitmap();
}

bool MenuView::IsItemEnabledForTesting(UINT id) const
{
	CHECK(IsInTest());

	const auto *item = GetItem(id);
	return item->IsEnabled();
}

bool MenuView::IsItemCheckedForTesting(UINT id) const
{
	CHECK(IsInTest());

	const auto *item = GetItem(id);
	return item->IsChecked();
}

const MenuView *MenuView::GetSubMenuViewForTesting(const MenuItemKey &key) const
{
	CHECK(IsInTest());

	auto id = MaybeGetIdForKey(key);
	CHECK(id);
	return GetSubMenuViewForTesting(*id);
}

const MenuView *MenuView::GetSubMenuViewForTesting(UINT id) const
{
	CHECK(IsInTest());

	auto itr = m_idToItemMap.find(id);
	CHECK(itr != m_idToItemMap.end());

	auto *subMenuView = itr->second.GetSubMenuView();
	CHECK(subMenuView);

	return subMenuView;
}

wil::com_ptr_nothrow<IDropTarget> MenuView::MaybeGetDropTargetForTesting(UINT id,
	MenuDropPosition position)
{
	CHECK(IsInTest());

	auto *item = GetItem(id);
	return item->MaybeGetDropTarget(position);
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

MenuView::Item::Item(MenuItemParams &params) :
	m_text(params.GetText()),
	m_iconModel(params.TakeIconModel()),
	m_helpText(params.GetHelpText()),
	m_acceleratorText(params.GetAcceleratorText()),
	m_enabledCallback(params.TakeEnabledCallback()),
	m_checkedCallback(params.TakeCheckedCallback()),
	m_selectedCallback(params.TakeSelectedCallback()),
	m_middleClickedCallback(params.TakeMiddleClickedCallback()),
	m_rightClickedCallback(params.TakeRightClickedCallback()),
	m_draggedCallback(params.TakeDraggedCallback()),
	m_maybeGetDropTargetCallback(params.TakeMaybeGetDropTargetCallback())
{
}

MenuView::Item::Item(SubMenuItemParams &params, std::unique_ptr<MenuView> subMenuView) :
	m_text(params.GetText()),
	m_iconModel(params.TakeIconModel()),
	m_subMenuView(std::move(subMenuView)),
	m_enabledCallback(params.TakeEnabledCallback()),
	m_middleClickedCallback(params.TakeMiddleClickedCallback()),
	m_rightClickedCallback(params.TakeRightClickedCallback()),
	m_draggedCallback(params.TakeDraggedCallback()),
	m_maybeGetDropTargetCallback(params.TakeMaybeGetDropTargetCallback())
{
}

void MenuView::Item::UpdateBitmap(wil::unique_hbitmap bitmap)
{
	m_bitmap = std::move(bitmap);
}

HBITMAP MenuView::Item::GetBitmap() const
{
	return m_bitmap.get();
}

const std::wstring &MenuView::Item::GetText() const
{
	return m_text;
}

const IconModel *MenuView::Item::GetIconModel() const
{
	return m_iconModel.get();
}

const std::wstring &MenuView::Item::GetHelpText() const
{
	return m_helpText;
}

const std::optional<std::wstring> &MenuView::Item::GetAcceleratorText() const
{
	return m_acceleratorText;
}

std::wstring MenuView::Item::GetFullText() const
{
	std::wstring fullText = m_text;

	if (m_acceleratorText)
	{
		fullText += L"\t" + *m_acceleratorText;
	}

	return fullText;
}

MenuView *MenuView::Item::GetSubMenuView()
{
	return m_subMenuView.get();
}

const MenuView *MenuView::Item::GetSubMenuView() const
{
	return m_subMenuView.get();
}

bool MenuView::Item::IsEnabled() const
{
	if (m_enabledCallback)
	{
		return m_enabledCallback();
	}

	return true;
}

bool MenuView::Item::IsChecked() const
{
	if (m_checkedCallback)
	{
		return m_checkedCallback();
	}

	return false;
}

void MenuView::Item::Select(bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (!IsEnabled())
	{
		return;
	}

	if (m_selectedCallback)
	{
		m_selectedCallback(isCtrlKeyDown, isShiftKeyDown);
	}
}

void MenuView::Item::MiddleClick(bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (!IsEnabled())
	{
		return;
	}

	if (m_middleClickedCallback)
	{
		m_middleClickedCallback(isCtrlKeyDown, isShiftKeyDown);
	}
}

void MenuView::Item::RightClick(const POINT &ptScreen)
{
	if (m_rightClickedCallback)
	{
		m_rightClickedCallback(ptScreen);
	}
}

MenuDragAction MenuView::Item::Drag()
{
	if (!IsEnabled())
	{
		return MenuDragAction::ContinueMenu;
	}

	if (m_draggedCallback)
	{
		return m_draggedCallback();
	}

	return MenuDragAction::ContinueMenu;
}

wil::com_ptr_nothrow<IDropTarget> MenuView::Item::MaybeGetDropTarget(MenuDropPosition position)
{
	if (m_maybeGetDropTargetCallback)
	{
		return m_maybeGetDropTargetCallback(position);
	}

	return nullptr;
}
