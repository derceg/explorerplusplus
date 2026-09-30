// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <oleidl.h>
#include <functional>

enum class MenuDragAction
{
	// Indicates that the menu should continue running on return.
	ContinueMenu,

	// Indicates that the menu should close on return.
	CloseMenu
};

enum class MenuDropPosition
{
	On,
	Before,
	After
};

using MenuItemEnabledCallback = std::function<bool()>;
using MenuItemCheckedCallback = std::function<bool()>;
using MenuItemSelectedCallback = std::function<void(bool isCtrlKeyDown, bool isShiftKeyDown)>;
using MenuItemMiddleClickedCallback = std::function<void(bool isCtrlKeyDown, bool isShiftKeyDown)>;
using MenuItemRightClickedCallback = std::function<void(const POINT &ptScreen)>;
using MenuItemDraggedCallback = std::function<MenuDragAction()>;
using MenuItemMaybeGetDropTargetCallback =
	std::function<wil::com_ptr_nothrow<IDropTarget>(MenuDropPosition position)>;
