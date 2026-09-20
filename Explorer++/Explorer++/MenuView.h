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
	MenuView *AppendSubMenu(MenuDelegate *delegate, UINT id, const std::wstring &text,
		std::unique_ptr<const IconModel> iconModel = {});
	void AppendSeparator();
	void EnableItem(UINT id, bool enable);
	void CheckItem(UINT id, bool check);
	void CheckRadioItem(UINT id, bool check);
	void RemoveTrailingSeparators();
	void ClearMenu();

	bool IsItemEnabled(UINT id) const;
	bool IsItemChecked(UINT id) const;
	int GetNumItems() const;

	std::wstring GetItemHelpText(UINT id) const;

	HMENU GetNativeMenuForTesting() const;
	UINT GetItemIdForTesting(int index) const;
	MenuDelegate *GetDelegateForItemForTesting(UINT id);
	std::wstring GetItemTextForTesting(UINT id) const;
	HBITMAP GetItemBitmapForTesting(UINT id) const;
	const MenuView *GetSubMenuViewForTesting(UINT id) const;
	void OnPopupWillShowForTesting(UINT dpi);
	void OnPopupClosedForTesting();

private:
	friend class MenuController;

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

	void OnPopupWillShowForDpi(UINT dpi);
	void OnPopupClosed();
	MenuView *MaybeGetMenuViewForNativeMenu(HMENU menu);
	MenuView *MaybeGetMenuViewForItem(UINT id);

	void SetItemImage(UINT id);
	void UpdateItemBitmap(UINT id, wil::unique_hbitmap bitmap);
	void MaybeAddImagesToMenu();
	UINT GetCurrentDpi() const;
	Item *GetItem(int id);
	const Item *GetItem(int id) const;

	UINT GetItemId(int index) const;
	MenuDelegate *GetDelegateForItem(UINT id);
	std::optional<UINT> MaybeGetItemAtPoint(const POINT &ptScreen) const;

	const HMENU m_menu;
	MenuView *m_parent = nullptr;
	std::unordered_map<UINT, std::unique_ptr<MenuView>> m_idToSubMenuMap;

	std::unordered_map<UINT, Item> m_idToItemMap;

	// This will only be set whilst the menu is being shown.
	std::optional<UINT> m_currentDpi;

	// If images have been added to the menu, this will indicate the DPI that was in effect at the
	// time. This can be used to detect a change in the DPI, allowing images to be re-added, if
	// necessary.
	//
	// If no images have been added yet (e.g. because the menu hasn't yet been shown), this value
	// will be empty.
	std::optional<UINT> m_lastRenderedImageDpi;

	WeakPtrFactory<MenuView> m_weakPtrFactory{ this };
};
