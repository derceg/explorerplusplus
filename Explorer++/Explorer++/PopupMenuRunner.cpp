// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PopupMenuRunner.h"
#include "../Helper/Helper.h"
#include "../Helper/MenuHelpTextHost.h"
#include "../Helper/MenuHelper.h"

PopupMenuRunner::PopupMenuRunner(MenuHelpTextHost *menuHelpTextHost) :
	m_ownedMenu(MenuHelper::CheckedCreatePopupMenu()),
	m_view(m_ownedMenu.get()),
	m_controller(&m_view, menuHelpTextHost),
	m_menuHelpTextHost(menuHelpTextHost)
{
}

MenuView *PopupMenuRunner::GetView()
{
	return &m_view;
}

void PopupMenuRunner::Show(HWND hwnd, const POINT &ptScreen)
{
	m_menuHelpTextHost->NotifyTopLevelMenuShown();
	m_controller.NotifyMenuWillShow(hwnd);

	// Without the TPM_RECURSE flag, TrackPopupMenu() will silently fail if another menu is
	// currently showing. It's hard to see how that would ever be the intended behavior. That is,
	// the caller has explicitly called Show(), to display the menu. Having the operation silently
	// fail because another menu is being shown isn't useful. Therefore, the TPM_RECURSE flag will
	// always be passed in.
	UINT cmd = TrackPopupMenu(m_ownedMenu.get(),
		TPM_LEFTALIGN | TPM_VERTICAL | TPM_RECURSE | TPM_RETURNCMD, ptScreen.x, ptScreen.y, 0, hwnd,
		nullptr);

	m_menuHelpTextHost->NotifyTopLevelMenuClosed();

	if (cmd == 0)
	{
		return;
	}

	// It's possible this menu was shown on top of another menu. If that was the case, the original
	// menu should be closed when an item from this menu has been selected.
	EndMenu();

	m_controller.SelectItem(cmd, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT));
}
