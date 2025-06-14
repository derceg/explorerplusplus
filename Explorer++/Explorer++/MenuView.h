// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconModel.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <memory>
#include <optional>
#include <unordered_map>

class MenuView
{
public:
	using ItemSelectedSignal =
		boost::signals2::signal<void(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown)>;
	using ItemMiddleClickedSignal =
		boost::signals2::signal<void(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown)>;
	using ViewDestroyedSignal = boost::signals2::signal<void()>;

	virtual ~MenuView();

	void AppendItem(UINT id, const std::wstring &text,
		std::unique_ptr<const IconModel> iconModel = {}, const std::wstring &helpText = L"",
		const std::optional<std::wstring> &acceleratorText = std::nullopt);
	void AppendSeparator();
	void EnableItem(UINT id, bool enable);
	void CheckItem(UINT id, bool check);
	void RemoveTrailingSeparators();
	void ClearMenu();
	std::wstring GetItemHelpText(UINT id) const;

	void SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);
	void MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);

	boost::signals2::connection AddItemSelectedObserver(
		const ItemSelectedSignal::slot_type &observer);
	boost::signals2::connection AddItemMiddleClickedObserver(
		const ItemMiddleClickedSignal::slot_type &observer);

	boost::signals2::connection AddViewDestroyedObserver(
		const ViewDestroyedSignal::slot_type &observer);

protected:
	virtual HMENU GetMenu() const = 0;

	void OnMenuWillShow(HWND ownerWindow);
	void OnMenuWillShowForDpi(UINT dpi);
	void OnMenuClosed();

private:
	void MaybeSetItemImage(UINT id, std::unique_ptr<const IconModel> iconModel);
	void SetItemImage(UINT id, const IconModel *iconModel);
	void UpdateItemBitmap(UINT id, wil::unique_hbitmap bitmap);
	void MaybeAddImagesToMenu();
	UINT GetCurrentDpi();

	std::unordered_map<UINT, std::unique_ptr<const IconModel>> m_itemIconModelMapping;
	std::unordered_map<UINT, wil::unique_hbitmap> m_itemImageMapping;
	std::unordered_map<UINT, std::wstring> m_itemHelpTextMapping;

	// This will only be set whilst the menu is being shown.
	std::optional<UINT> m_currentDpi;

	// If images have been added to the menu, this will indicate the DPI that was in effect at the
	// time. This can be used to detect a change in the DPI, allowing images to be re-added, if
	// necessary.
	//
	// If no images have been added yet (e.g. because the menu hasn't yet been shown), this value
	// will be empty.
	std::optional<UINT> m_lastRenderedImageDpi;

	ItemSelectedSignal m_itemSelectedSignal;
	ItemMiddleClickedSignal m_itemMiddleClickedSignal;
	ViewDestroyedSignal m_viewDestroyedSignal;

	WeakPtrFactory<MenuView> m_weakPtrFactory{ this };
};
