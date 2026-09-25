// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconModel.h"
#include "MenuDelegate.h"
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

	void AppendItem(MenuDelegate *delegate, UINT id, const std::wstring &text,
		std::unique_ptr<const IconModel> iconModel = {}, const std::wstring &helpText = L"",
		const std::optional<std::wstring> &acceleratorText = std::nullopt);
	void AppendRadioItem(MenuDelegate *delegate, UINT id, const std::wstring &text,
		std::unique_ptr<const IconModel> iconModel = {}, const std::wstring &helpText = L"",
		const std::optional<std::wstring> &acceleratorText = std::nullopt);
	MenuView *AppendSubMenu(MenuDelegate *delegate, UINT id, const std::wstring &text,
		std::unique_ptr<const IconModel> iconModel = {});
	void AppendSeparator();
	void RemoveDuplicateSeparators();
	void RemoveTrailingSeparators();
	void ClearMenu();

	int GetNumItems() const;

	std::wstring GetItemHelpText(UINT id) const;

	HMENU GetNativeMenuForTesting() const;
	UINT GetItemIdForTesting(int index) const;
	MenuDelegate *MaybeGetDelegateForItemForTesting(UINT id);
	const MenuDelegate *MaybeGetDelegateForItemForTesting(UINT id) const;
	std::wstring GetItemTextForTesting(UINT id) const;
	HBITMAP GetItemBitmapForTesting(UINT id) const;
	const MenuView *GetSubMenuViewForTesting(UINT id) const;
	void OnPopupWillShowForTesting(UINT dpi);
	void OnPopupClosedForTesting();

private:
	friend class MenuController;

	enum class CheckStyle
	{
		Normal,
		Radio
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

		MenuDelegate *const delegate;
		const std::unique_ptr<const IconModel> iconModel;
		wil::unique_hbitmap bitmap;
		const std::wstring helpText;
	};

	void AppendItem(MenuDelegate *delegate, UINT id, const std::wstring &text,
		std::unique_ptr<const IconModel> iconModel, const std::wstring &helpText,
		const std::optional<std::wstring> &acceleratorText, CheckStyle checkStyle);

	void OnPopupWillShowForDpi(UINT dpi);
	void OnPopupClosed();
	MenuView *MaybeGetMenuViewForNativeMenu(HMENU menu);
	MenuView *MaybeGetMenuViewForItem(UINT id);
	const MenuView *MaybeGetMenuViewForItem(UINT id) const;

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
	std::unordered_map<UINT, std::unique_ptr<MenuView>> m_idToSubMenuMap;

	std::unordered_map<UINT, Item> m_idToItemMap;

	bool m_isShowing = false;

	// This will only be set after the menu is first shown.
	std::optional<UINT> m_currentDpi;

	WeakPtrFactory<MenuView> m_weakPtrFactory{ this };
};
