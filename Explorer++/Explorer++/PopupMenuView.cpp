// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PopupMenuView.h"
#include "../Helper/Helper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowSubclass.h"

PopupMenuView::PopupMenuView() : m_menu(CreatePopupMenu())
{
}

void PopupMenuView::Show(HWND hwnd, const POINT &point)
{
	DCHECK_GT(GetMenuItemCount(m_menu.get()), 0);

	// Subclass the parent window to allow middle clicks to be detected.
	auto subclass = std::make_unique<WindowSubclass>(hwnd,
		std::bind_front(&PopupMenuView::ParentWindowSubclass, this));

	UINT cmd = TrackPopupMenu(m_menu.get(), TPM_LEFTALIGN | TPM_VERTICAL | TPM_RETURNCMD, point.x,
		point.y, 0, hwnd, nullptr);

	if (cmd == 0)
	{
		return;
	}

	SelectItem(cmd, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT));
}

LRESULT PopupMenuView::ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MBUTTONUP:
		OnMenuMiddleButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			WI_IsFlagSet(wParam, MK_CONTROL), WI_IsFlagSet(wParam, MK_SHIFT));
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void PopupMenuView::OnMenuMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	auto menuItemId = MenuHelper::MaybeGetMenuItemAtPoint(m_menu.get(), pt);

	if (!menuItemId)
	{
		return;
	}

	MiddleClickItem(*menuItemId, isCtrlKeyDown, isShiftKeyDown);
}

HMENU PopupMenuView::GetMenu() const
{
	return m_menu.get();
}
