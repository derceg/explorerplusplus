// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuItemCallbacks.h"
#include <wil/com.h>
#include <oleidl.h>

struct MenuDropLocation
{
	UINT id;
	MenuDropPosition position;

	bool operator==(const MenuDropLocation &) const = default;
};

class MenuDelegate
{
public:
	virtual ~MenuDelegate() = default;

	virtual bool IsItemEnabled(UINT id) const
	{
		UNREFERENCED_PARAMETER(id);

		return true;
	}

	virtual bool IsItemChecked(UINT id) const
	{
		UNREFERENCED_PARAMETER(id);

		return false;
	}

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
