// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellContextMenuDelegateFake.h"

void ShellContextMenuDelegateFake::UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, ShellContextMenuBuilder *builder)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(builder);
}

bool ShellContextMenuDelegateFake::MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, const std::wstring &verb)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(verb);

	return false;
}

void ShellContextMenuDelegateFake::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, UINT menuItemId)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(menuItemId);
}

std::wstring ShellContextMenuDelegateFake::GetHelpTextForCustomItem(UINT menuItemId)
{
	UNREFERENCED_PARAMETER(menuItemId);

	return L"";
}
