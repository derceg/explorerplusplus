// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MenuTestHost.h"
#include "../Helper/MenuHelper.h"

MenuTestHost::MenuTestHost(MenuHelpTextHost *menuHelpTextHost) :
	m_ownedMenu(MenuHelper::CheckedCreatePopupMenu()),
	m_view(m_ownedMenu.get()),
	m_controller(&m_view, nullptr, menuHelpTextHost)
{
}

MenuView *MenuTestHost::GetView()
{
	return &m_view;
}

void MenuTestHost::SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	m_controller.SelectItem(id, isCtrlKeyDown, isShiftKeyDown);
}

void MenuTestHost::MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	m_controller.MiddleClickItem(id, isCtrlKeyDown, isShiftKeyDown);
}

void MenuTestHost::SelectItemAtIndex(int index, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	m_controller.SelectItem(m_view.GetItemIdForTesting(index), isCtrlKeyDown, isShiftKeyDown);
}

void MenuTestHost::MiddleClickItemAtIndex(int index, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	m_controller.MiddleClickItem(m_view.GetItemIdForTesting(index), isCtrlKeyDown, isShiftKeyDown);
}
