// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBackgroundContextMenuDelegate.h"

void ShellBackgroundContextMenuDelegate::UpdateMenuEntries(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, ShellContextMenuBuilder *builder)
{
	UNREFERENCED_PARAMETER(items);

	UpdateMenuEntries(directory, builder);
}

bool ShellBackgroundContextMenuDelegate::MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, const std::wstring &verb)
{
	UNREFERENCED_PARAMETER(items);

	return MaybeHandleShellMenuItem(directory, verb);
}

void ShellBackgroundContextMenuDelegate::HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, UINT menuItemId)
{
	UNREFERENCED_PARAMETER(items);

	HandleCustomMenuItem(directory, menuItemId);
}
