// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
// clang-format off
#include "ShellTreeViewContextMenuDelegate.h"
// clang-format on
#include "ShellTreeView.h"
#include "../Helper/Pidl.h"

ShellTreeViewContextMenuDelegate::ShellTreeViewContextMenuDelegate(ShellTreeView *shellTreeView) :
	m_shellTreeView(shellTreeView)
{
}

void ShellTreeViewContextMenuDelegate::UpdateMenuEntries(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, ShellContextMenuBuilder *builder)
{
	UNREFERENCED_PARAMETER(directory);
	UNREFERENCED_PARAMETER(items);
	UNREFERENCED_PARAMETER(builder);
}

bool ShellTreeViewContextMenuDelegate::MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, const std::wstring &verb)
{
	// The context menu should only ever be shown for a single item in the treeview.
	CHECK_EQ(items.size(), 1u);

	auto pidlComplete = directory + items[0];

	if (verb == L"rename")
	{
		m_shellTreeView->StartRenamingItem(pidlComplete.Raw());

		return true;
	}
	else if (verb == L"copy")
	{
		m_shellTreeView->CopyItemToClipboard(pidlComplete.Raw(), ClipboardAction::Copy);

		return true;
	}
	else if (verb == L"cut")
	{
		m_shellTreeView->CopyItemToClipboard(pidlComplete.Raw(), ClipboardAction::Cut);

		return true;
	}

	return false;
}

void ShellTreeViewContextMenuDelegate::HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory,
	const std::vector<PidlChild> &items, UINT menuItemId)
{
	UNREFERENCED_PARAMETER(directory);
	UNREFERENCED_PARAMETER(items);
	UNREFERENCED_PARAMETER(menuItemId);
}

std::wstring ShellTreeViewContextMenuDelegate::GetHelpTextForCustomItem(UINT menuItemId)
{
	UNREFERENCED_PARAMETER(menuItemId);

	return L"";
}
