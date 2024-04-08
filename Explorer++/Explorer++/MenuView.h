// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <vector>

class MenuView
{
public:
	using ItemSelectedSignal =
		boost::signals2::signal<void(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown)>;
	using ItemMiddleClickedSignal =
		boost::signals2::signal<void(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown)>;
	using ViewDestroyedSignal = boost::signals2::signal<void()>;

	virtual ~MenuView();

	void AppendItem(UINT id, const std::wstring &text, wil::unique_hbitmap bitmap = nullptr);
	void SetBitmapForItem(UINT id, wil::unique_hbitmap bitmap);

	void SelectItem(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);
	void MiddleClickItem(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown);

	boost::signals2::connection AddItemSelectedObserver(
		const ItemSelectedSignal::slot_type &observer);
	boost::signals2::connection AddItemMiddleClickedObserver(
		const ItemMiddleClickedSignal::slot_type &observer);

	boost::signals2::connection AddViewDestroyedObserver(
		const ViewDestroyedSignal::slot_type &observer);

	int GetItemCountForTesting() const;
	UINT GetItemIdForTesting(int index) const;
	std::wstring GetItemTextForTesting(UINT id) const;
	HBITMAP GetItemBitmapForTesting(UINT id) const;

private:
	virtual HMENU GetMenu() const = 0;

	std::vector<wil::unique_hbitmap> m_menuImages;
	ItemSelectedSignal m_itemSelectedSignal;
	ItemMiddleClickedSignal m_itemMiddleClickedSignal;
	ViewDestroyedSignal m_viewDestroyedSignal;
};
