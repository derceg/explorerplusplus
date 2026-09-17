// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <oleidl.h>

struct MenuDropLocation
{
	enum class Position
	{
		On,
		Before,
		After
	};

	UINT id;
	Position position;

	bool operator==(const MenuDropLocation &) const = default;
};

enum class MenuDragAction
{
	// Indicates that the menu should continue running on return.
	ContinueMenu,

	// Indicates that the menu should close on return.
	CloseMenu
};

class MenuDelegate
{
public:
	virtual ~MenuDelegate() = default;

	virtual void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) = 0;

	virtual void OnItemMiddleClicked(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
	{
		UNREFERENCED_PARAMETER(id);
		UNREFERENCED_PARAMETER(isCtrlKeyDown);
		UNREFERENCED_PARAMETER(isShiftKeyDown);
	}

	virtual void OnItemRightClicked(UINT id, const POINT &ptScreen)
	{
		UNREFERENCED_PARAMETER(id);
		UNREFERENCED_PARAMETER(ptScreen);
	}

	virtual MenuDragAction OnItemDragged(UINT id)
	{
		UNREFERENCED_PARAMETER(id);

		return MenuDragAction::ContinueMenu;
	}

	virtual wil::com_ptr_nothrow<IDropTarget> MaybeGetDropTargetForLocation(
		const MenuDropLocation &dropLocation)
	{
		UNREFERENCED_PARAMETER(dropLocation);

		return nullptr;
	}
};
