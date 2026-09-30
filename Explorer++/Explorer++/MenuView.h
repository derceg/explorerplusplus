// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconModel.h"
#include "MenuDelegate.h"
#include "MenuItemKey.h"
#include "MenuItemParams.h"
#include "SubMenuItemParams.h"
#include "../Helper/WeakPtrFactory.h"
#include <wil/resource.h>
#include <memory>
#include <optional>
#include <unordered_map>

class MenuView
{
public:
	MenuView(HMENU menu);

	bool IsRoot() const;

	void EnableDragAndDrop(bool enable);

	void AppendItem(MenuItemParams params);
	MenuView *AppendSubMenu(SubMenuItemParams params);

	void AppendItem(MenuDelegate *delegate, UINT id, const std::wstring &text,
		std::unique_ptr<const IconModel> iconModel = {}, const std::wstring &helpText = L"",
		const std::optional<std::wstring> &acceleratorText = std::nullopt);
	void AppendSeparator();
	void RemoveDuplicateSeparators();
	void RemoveTrailingSeparators();
	void ClearMenu();

	int GetNumItems() const;

	std::wstring GetItemHelpText(UINT id) const;

	HMENU GetNativeMenuForTesting() const;
	UINT GetItemIdForTesting(int index) const;
	std::optional<UINT> MaybeGetIdForKeyForTesting(const MenuItemKey &key) const;
	MenuDelegate *MaybeGetDelegateForItemForTesting(UINT id);
	const MenuDelegate *MaybeGetDelegateForItemForTesting(UINT id) const;
	std::wstring GetItemTextForTesting(UINT id) const;
	HBITMAP GetItemBitmapForTesting(UINT id) const;
	const MenuView *GetSubMenuViewForTesting(const MenuItemKey &key) const;
	const MenuView *GetSubMenuViewForTesting(UINT id) const;
	void OnPopupWillShowForTesting(UINT dpi);
	void OnPopupClosedForTesting();

private:
	friend class MenuController;

	class InternalMenuDelegate : public MenuDelegate
	{
	public:
		InternalMenuDelegate(MenuItemParams &params);
		InternalMenuDelegate(SubMenuItemParams &params);

		// MenuDelegate
		bool IsItemEnabled(UINT id) const override;
		bool IsItemChecked(UINT id) const override;
		void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override;
		void OnItemMiddleClicked(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override;
		void OnItemRightClicked(UINT id, const POINT &ptScreen) override;
		MenuDragAction OnItemDragged(UINT id) override;
		wil::com_ptr_nothrow<IDropTarget> MaybeGetDropTargetForLocation(
			const MenuDropLocation &dropLocation) override;

	private:
		MenuItemEnabledCallback m_enabledCallback;
		MenuItemCheckedCallback m_checkedCallback;
		MenuItemSelectedCallback m_selectedCallback;
		MenuItemMiddleClickedCallback m_middleClickedCallback;
		MenuItemRightClickedCallback m_rightClickedCallback;
		MenuItemDraggedCallback m_draggedCallback;
		MenuItemMaybeGetDropTargetCallback m_maybeGetDropTargetCallback;
	};

	struct Item
	{
		Item(MenuDelegate *delegate, std::unique_ptr<const IconModel> iconModel,
			const std::wstring &helpText) :
			delegate(delegate),
			iconModel(std::move(iconModel)),
			helpText(helpText)
		{
		}

		std::unique_ptr<InternalMenuDelegate> ownedDelegate;
		MenuDelegate *const delegate;
		const std::unique_ptr<const IconModel> iconModel;
		wil::unique_hbitmap bitmap;
		const std::wstring helpText;
	};

	void AppendItem(MenuDelegate *delegate, UINT id, const std::wstring &text,
		std::unique_ptr<const IconModel> iconModel, const std::wstring &helpText,
		const std::optional<std::wstring> &acceleratorText, MenuItemCheckStyle checkStyle);
	UINT AllocateId();
	MenuView *GetRoot();

	void OnPopupWillShowForDpi(UINT dpi);
	void OnPopupClosed();
	MenuView *MaybeGetMenuViewForNativeMenu(HMENU menu);
	MenuView *MaybeGetMenuViewForItem(UINT id);
	const MenuView *MaybeGetMenuViewForItem(UINT id) const;
	std::optional<UINT> MaybeGetIdForKey(const MenuItemKey &key) const;

	void ClearItemImages();
	void UpdateDynamicPropertiesForAllItems();
	void UpdateDynamicPropertiesForItem(UINT id);
	void UpdateItemState(UINT id);
	void SetItemImage(UINT id);
	void UpdateItemBitmap(UINT id, wil::unique_hbitmap bitmap);
	UINT GetCurrentDpi() const;
	Item *GetItem(int id);
	const Item *GetItem(int id) const;

	std::optional<UINT> MaybeGetItemId(int index) const;
	MenuDelegate *MaybeGetDelegateForItem(UINT id);
	const MenuDelegate *MaybeGetDelegateForItem(UINT id) const;
	std::optional<UINT> MaybeGetItemAtPoint(const POINT &ptScreen) const;

	const HMENU m_menu;
	MenuView *m_parent = nullptr;

	// This exists on every MenuView instance, but is only used on the root menu. That is, IDs are
	// always generated from the root menu.
	UINT m_idCounter = 1;

	std::unordered_map<UINT, std::unique_ptr<MenuView>> m_idToSubMenuMap;
	std::unordered_map<UINT, Item> m_idToItemMap;
	std::unordered_map<MenuItemKey, UINT, boost::hash<MenuItemKey>> m_keyToIdMap;

	bool m_isShowing = false;

	// This will only be set after the menu is first shown.
	std::optional<UINT> m_currentDpi;

	WeakPtrFactory<MenuView> m_weakPtrFactory{ this };
};
