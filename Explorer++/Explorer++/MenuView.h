// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconModel.h"
#include "MenuDelegate.h"
#include "MenuItemKey.h"
#include "MenuItemParams.h"
#include "SubMenuItemParams.h"
#include "../Helper/PassKey.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <memory>
#include <optional>
#include <unordered_map>

class MenuView
{
private:
	using PassKey = PassKey<MenuView>;

public:
	using PopupWillShowSignal = boost::signals2::signal<void()>;

	// This is used to construct a new root menu.
	MenuView(HMENU menu);

	// This is used to construct a new submenu and is only used internally (external callers should
	// use AppendSubMenu()).
	MenuView(MenuView *parent, HMENU menu, PassKey);

	bool IsRoot() const;

	void AppendItem(MenuItemParams params);
	MenuView *AppendSubMenu(SubMenuItemParams params);

	void AppendSeparator();
	void RemoveDuplicateSeparators();
	void RemoveTrailingSeparators();
	void ClearMenu();

	boost::signals2::connection AddPopupWillShowObserver(
		const PopupWillShowSignal::slot_type &observer);

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

	// Note that to avoid potential overlap between IDs generated here and existing accelerator IDs,
	// the ID range here starts from outside the 16-bit range. That is, when WM_COMMAND is
	// dispatched, the accelerator ID wil be stored in the low word of wParam. That constrains
	// accelerator values to be 16-bit. So, a value above that range is guaranteed not to be an
	// accelerator.
	//
	// Avoiding accelerator ID overlap is important, since whether an accelerator is dispatched
	// depends on whether the associated item in the main menu is enabled or not. If the main menu
	// contains an item with a particular ID and there is an accelerator with the same ID, the
	// accelerator won't be dispatched if the menu item is disabled.
	//
	// This has some implications for menu functionality. Although menu item IDs are UINTs, there
	// are some places where they're truncated to 16-bit values. WM_COMMAND is one of those places,
	// so that shouldn't be used to handle item selection. Additionally, WM_MENUSELECT also
	// truncates, so it can require special handling.
	static constexpr UINT INITIAL_ID = 0x10000;

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

	MenuView *const m_parent;
	const HMENU m_menu;

	// This exists on every MenuView instance, but is only used on the root menu. That is, IDs are
	// always generated from the root menu.
	UINT m_idCounter = INITIAL_ID;

	std::unordered_map<UINT, std::unique_ptr<MenuView>> m_idToSubMenuMap;
	std::unordered_map<UINT, Item> m_idToItemMap;
	std::unordered_map<MenuItemKey, UINT, boost::hash<MenuItemKey>> m_keyToIdMap;

	PopupWillShowSignal m_popupWillShowSignal;
	bool m_isShowing = false;

	// This will only be set after the menu is first shown.
	std::optional<UINT> m_currentDpi;

	WeakPtrFactory<MenuView> m_weakPtrFactory{ this };
};
