// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
// clang-format off
#include "ShellTreeViewContextMenuDelegate.h"
// clang-format on
#include "ShellTreeView.h"
#include "../Helper/PidlHelper.h"

ShellTreeViewContextMenuDelegate::ShellTreeViewContextMenuDelegate(ShellTreeView *shellTreeView) :
	m_shellTreeView(shellTreeView)
{
}

void ShellTreeViewContextMenuDelegate::UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, ShellContextMenuBuilder *builder)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(builder);
}

bool ShellTreeViewContextMenuDelegate::MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, const std::wstring &verb)
{
	// The context menu should only ever be shown for a single item in the treeview.
	CHECK_EQ(pidlItems.size(), 1u);

	PidlAbsolute pidlComplete = CombinePidls(pidlParent, pidlItems[0].Raw());

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

void ShellTreeViewContextMenuDelegate::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, UINT menuItemId)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(menuItemId);
}

std::wstring ShellTreeViewContextMenuDelegate::GetHelpTextForCustomItem(UINT menuItemId)
{
	UNREFERENCED_PARAMETER(menuItemId);

	return L"";
}
