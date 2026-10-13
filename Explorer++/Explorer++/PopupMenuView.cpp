// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PopupMenuView.h"
#include "../Helper/Helper.h"
#include "../Helper/MenuHelper.h"

PopupMenuView::PopupMenuView(HWND ownerWindow, MenuHelpTextHost *menuHelpTextHost) :
	RootMenuView(MenuHelper::CheckedCreatePopupMenu(), ownerWindow, MenuType::ContextMenu,
		menuHelpTextHost),
	m_ownerWindow(ownerWindow)
{
}

void PopupMenuView::Show(const POINT &ptScreen)
{
	// Without the TPM_RECURSE flag, TrackPopupMenu() will silently fail if another menu is
	// currently showing. It's hard to see how that would ever be the intended behavior. That is,
	// the caller has explicitly called Show(), to display the menu. Having the operation silently
	// fail because another menu is being shown isn't useful. Therefore, the TPM_RECURSE flag will
	// always be passed in.
	UINT cmd = TrackPopupMenu(m_menu,
		(GetSystemMetrics(SM_MENUDROPALIGNMENT) == 0 ? TPM_LEFTALIGN : TPM_RIGHTALIGN)
			| TPM_VERTICAL | TPM_RECURSE | TPM_RETURNCMD,
		ptScreen.x, ptScreen.y, 0, m_ownerWindow, nullptr);

	if (cmd == 0)
	{
		return;
	}

	// It's possible this menu was shown on top of another menu. If that was the case, the original
	// menu should be closed when an item from this menu has been selected.
	EndMenu();

	SelectItem(cmd, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT));
}
