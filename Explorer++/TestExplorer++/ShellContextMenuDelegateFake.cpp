// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellContextMenuDelegateFake.h"

void ShellContextMenuDelegateFake::UpdateMenuEntries(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, ShellContextMenuBuilder *builder)
{
	UNREFERENCED_PARAMETER(directory);
	UNREFERENCED_PARAMETER(items);
	UNREFERENCED_PARAMETER(builder);
}

bool ShellContextMenuDelegateFake::MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, const std::wstring &verb)
{
	UNREFERENCED_PARAMETER(directory);
	UNREFERENCED_PARAMETER(items);
	UNREFERENCED_PARAMETER(verb);

	return false;
}

void ShellContextMenuDelegateFake::HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, UINT menuItemId)
{
	UNREFERENCED_PARAMETER(directory);
	UNREFERENCED_PARAMETER(items);
	UNREFERENCED_PARAMETER(menuItemId);
}

std::wstring ShellContextMenuDelegateFake::GetHelpTextForCustomItem(UINT menuItemId)
{
	UNREFERENCED_PARAMETER(menuItemId);

	return L"";
}
