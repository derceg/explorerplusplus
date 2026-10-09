// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconModel.h"
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

// If a menu adds external menu items, this interface can be used to manage events for those items.
// An external item refers to an item added by non-application code (e.g. by the shell).
class ExternalMenuItemDelegate
{
public:
	virtual ~ExternalMenuItemDelegate() = default;

	virtual std::optional<LRESULT> MaybeHandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	virtual std::optional<std::wstring> MaybeGetExternalItemHelpText(UINT id) const = 0;
	virtual void OnExternalItemSelected(UINT id) = 0;
};

class MenuView
{
private:
	using PassKey = PassKey<MenuView>;

public:
	using PopupWillShowSignal = boost::signals2::signal<void()>;

	// IDs in this range can be assigned to external menu items (e.g. shell context menu items) and
	// will be handled differently from normal items. For example, it's only possible to select
	// external items; middle-clicking and right-clicking an external item will have no effect.
	// Additionally, item selection will be handled by the supplied ExternalMenuItemDelegate.
	//
	// Note that the range values are both inclusive.
	static constexpr UINT EXTERNAL_ID_MIN = 1;
	static constexpr UINT EXTERNAL_ID_MAX = 0xFFFF;

	// This is used to construct a new root menu.
	MenuView(HMENU menu);

	// This is used to construct a new submenu and is only used internally (external callers should
	// use AppendSubMenu()).
	MenuView(MenuView *parent, HMENU menu, PassKey);

	bool IsRoot() const;

	void SetExternalItemDelegate(ExternalMenuItemDelegate *externalItemDelegate);

	void AppendItem(MenuItemParams params);
	void AddItem(MenuItemParams params, UINT index);
	MenuView *AppendSubMenu(SubMenuItemParams params);

	void AppendSeparator();
	void RemoveDuplicateSeparators();
	void RemoveTrailingSeparators();
	void ClearMenu();

	boost::signals2::connection AddPopupWillShowObserver(
		const PopupWillShowSignal::slot_type &observer);

	int GetNumItems() const;

	std::wstring GetItemHelpText(UINT id) const;
	void SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);
	void MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);
	void RightClickItem(UINT id, const POINT &ptScreen);
	MenuDragAction DragItem(UINT id);
	wil::com_ptr_nothrow<IDropTarget> MaybeGetDropTargetForItem(UINT id, MenuDropPosition position);

	// When using this to add external items, items should only be added within [EXTERNAL_ID_MIN,
	// EXTERNAL_ID_MAX]. Adding items outside that range or otherwise altering the menu is invalid
	// and will cause issues.
	HMENU GetNativeMenuForExternalItems();

	HMENU GetNativeMenuForTesting() const;
	UINT GetItemIdForTesting(int index) const;
	std::optional<UINT> MaybeGetIdForKeyForTesting(const MenuItemKey &key) const;
	std::wstring GetItemFullTextForTesting(UINT id) const;
	HBITMAP GetItemBitmapForTesting(UINT id) const;
	bool IsItemEnabledForTesting(UINT id) const;
	bool IsItemCheckedForTesting(UINT id) const;
	const MenuView *GetSubMenuViewForTesting(const MenuItemKey &key) const;
	const MenuView *GetSubMenuViewForTesting(UINT id) const;
	wil::com_ptr_nothrow<IDropTarget> MaybeGetDropTargetForTesting(UINT id,
		MenuDropPosition position);
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
	static constexpr UINT INITIAL_ID = EXTERNAL_ID_MAX + 1;

	class Item
	{
	public:
		Item(MenuItemParams &params);
		Item(SubMenuItemParams &params, std::unique_ptr<MenuView> subMenuView);

		Item(const Item &) = delete;
		Item &operator=(const Item &) = delete;
		Item(Item &&) = default;
		Item &operator=(Item &&) = default;

		void UpdateBitmap(wil::unique_hbitmap bitmap);
		HBITMAP GetBitmap() const;

		const std::wstring &GetText() const;
		const IconModel *GetIconModel() const;
		const std::wstring &GetHelpText() const;
		const std::optional<std::wstring> &GetAcceleratorText() const;
		std::wstring GetFullText() const;

		MenuView *GetSubMenuView();
		const MenuView *GetSubMenuView() const;

		bool IsEnabled() const;
		bool IsChecked() const;

		void Select(bool isCtrlKeyDown, bool isShiftKeyDown);
		void MiddleClick(bool isCtrlKeyDown, bool isShiftKeyDown);
		void RightClick(const POINT &ptScreen);
		MenuDragAction Drag();
		wil::com_ptr_nothrow<IDropTarget> MaybeGetDropTarget(MenuDropPosition position);

	private:
		std::wstring m_text;
		std::unique_ptr<const IconModel> m_iconModel;
		wil::unique_hbitmap m_bitmap;
		std::wstring m_helpText;
		std::optional<std::wstring> m_acceleratorText;
		std::unique_ptr<MenuView> m_subMenuView;

		MenuItemEnabledCallback m_enabledCallback;
		MenuItemCheckedCallback m_checkedCallback;
		MenuItemSelectedCallback m_selectedCallback;
		MenuItemMiddleClickedCallback m_middleClickedCallback;
		MenuItemRightClickedCallback m_rightClickedCallback;
		MenuItemDraggedCallback m_draggedCallback;
		MenuItemMaybeGetDropTargetCallback m_maybeGetDropTargetCallback;
	};

	UINT AllocateId();
	MenuView *GetRoot();
	const MenuView *GetRoot() const;
	Item *RegisterItem(UINT id, Item item, const std::optional<MenuItemKey> &key);

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
	std::wstring GetExternalItemHelpText(UINT id) const;
	void SelectExternalItem(UINT id);
	static bool IsExternalItem(UINT id);
	ExternalMenuItemDelegate *MaybeGetExternalItemDelegate();
	const ExternalMenuItemDelegate *MaybeGetExternalItemDelegate() const;
	Item *GetItem(int id);
	const Item *GetItem(int id) const;

	std::optional<UINT> MaybeGetItemId(int index) const;
	std::optional<UINT> MaybeGetItemAtPoint(const POINT &ptScreen) const;

	MenuView *const m_parent;
	const HMENU m_menu;
	ExternalMenuItemDelegate *m_externalItemDelegate = nullptr;

	// This exists on every MenuView instance, but is only used on the root menu. That is, IDs are
	// always generated from the root menu.
	UINT m_idCounter = INITIAL_ID;

	std::unordered_map<UINT, Item> m_idToItemMap;
	std::unordered_map<MenuItemKey, UINT, boost::hash<MenuItemKey>> m_keyToIdMap;

	PopupWillShowSignal m_popupWillShowSignal;
	bool m_isShowing = false;

	// This will only be set after the menu is first shown.
	std::optional<UINT> m_currentDpi;

	WeakPtrFactory<MenuView> m_weakPtrFactory{ this };
};
