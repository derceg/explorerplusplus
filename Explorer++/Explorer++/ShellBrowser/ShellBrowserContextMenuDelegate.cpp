// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserContextMenuDelegate.h"
#include "ShellBrowserImpl.h"

ShellBrowserContextMenuDelegate::ShellBrowserContextMenuDelegate(
	WeakPtr<ShellBrowserImpl> shellBrowserWeak) :
	m_shellBrowserWeak(shellBrowserWeak)
{
}

void ShellBrowserContextMenuDelegate::UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, ShellContextMenuBuilder *builder)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(builder);
}

bool ShellBrowserContextMenuDelegate::MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, const std::wstring &verb)
{
	if (!m_shellBrowserWeak)
	{
		return false;
	}

	if (verb == L"rename")
	{
		m_shellBrowserWeak->StartRenamingItems(BuildItemList(pidlParent, pidlItems));

		return true;
	}
	else if (verb == L"copy")
	{
		m_shellBrowserWeak->CopyItemsToClipboard(BuildItemList(pidlParent, pidlItems),
			ClipboardAction::Copy);

		return true;
	}
	else if (verb == L"cut")
	{
		m_shellBrowserWeak->CopyItemsToClipboard(BuildItemList(pidlParent, pidlItems),
			ClipboardAction::Cut);

		return true;
	}

	return false;
}

std::vector<PidlAbsolute> ShellBrowserContextMenuDelegate::BuildItemList(
	PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems) const
{
	std::vector<PidlAbsolute> fullPidls;

	for (const auto &child : pidlItems)
	{
		fullPidls.push_back(CombinePidls(pidlParent, child.Raw()));
	}

	return fullPidls;
}

void ShellBrowserContextMenuDelegate::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PidlChild> &pidlItems, UINT menuItemId)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(menuItemId);
}

std::wstring ShellBrowserContextMenuDelegate::GetHelpTextForCustomItem(UINT menuItemId)
{
	UNREFERENCED_PARAMETER(menuItemId);

	return L"";
}
