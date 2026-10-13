// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MenuHelpTextHostFake.h"

void MenuHelpTextHostFake::NotifyTopLevelMenuShown()
{
}

void MenuHelpTextHostFake::NotifyTopLevelMenuClosed()
{
}

void MenuHelpTextHostFake::MenuItemSelected(HMENU menu, WORD indexOrId, UINT flags)
{
	UNREFERENCED_PARAMETER(menu);
	UNREFERENCED_PARAMETER(indexOrId);
	UNREFERENCED_PARAMETER(flags);
}

boost::signals2::connection MenuHelpTextHostFake::AddMenuHelpTextRequestObserver(
	const MenuHelpTextRequestSignal::slot_type &observer)
{
	return m_menuHelpTextRequestSignal.connect(observer);
}

std::optional<std::wstring> MenuHelpTextHostFake::TriggerHelpTextRequest(HMENU menu, UINT id)
{
	return m_menuHelpTextRequestSignal(menu, id);
}
