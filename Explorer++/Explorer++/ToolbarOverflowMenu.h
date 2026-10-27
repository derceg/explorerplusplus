// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "../Helper/WeakPtr.h"
#include <wil/com.h>
#include <commoncontrols.h>
#include <memory>
#include <vector>

class ToolbarButton;
class ToolbarItem;

class ToolbarOverflowMenu : public MenuBase
{
public:
	ToolbarOverflowMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const std::vector<ToolbarItem> &overflowItems,
		const wil::com_ptr_nothrow<IImageList> &imageList);

private:
	void BuildMenu(const std::vector<ToolbarItem> &overflowItems,
		const wil::com_ptr_nothrow<IImageList> &imageList);
	void AddButtonToMenu(WeakPtr<ToolbarButton> button,
		const wil::com_ptr_nothrow<IImageList> &imageList);

	static void OnItemSelected(WeakPtr<ToolbarButton> button, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	static void OnItemMiddleClicked(WeakPtr<ToolbarButton> button, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	static void OnItemRightClicked(WeakPtr<ToolbarButton> button, const POINT &ptScreen);

	std::vector<std::unique_ptr<MenuBase>> m_childMenus;
};
