// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellItemContextMenu.h"
#include "ShellHelper.h"
#include "ShellItemContextMenuDelegate.h"
#include <wil/common.h>
#include <algorithm>
#include <iterator>

ShellItemContextMenu::ShellItemContextMenu(PCIDLIST_ABSOLUTE directory,
	const std::vector<PCITEMID_CHILD> &items, MenuHelpTextHost *menuHelpTextHost) :
	ShellContextMenu(directory, items, menuHelpTextHost)
{
	CHECK(!m_items.empty());
}

void ShellItemContextMenu::AddDelegate(ShellItemContextMenuDelegate *delegate)
{
	ShellContextMenu::AddDelegate(delegate);
}

void ShellItemContextMenu::ShowMenu(HWND hwnd, const POINT *pt, IUnknown *site, Flags flags)
{
	UINT contextMenuflags = CMF_NORMAL;

	if (WI_IsFlagSet(flags, Flags::ExtendedVerbs))
	{
		contextMenuflags |= CMF_EXTENDEDVERBS;
	}

	if (WI_IsFlagSet(flags, Flags::Rename))
	{
		contextMenuflags |= CMF_CANRENAME;
	}

	ShellContextMenu::ShowMenu(hwnd, pt, site, contextMenuflags);
}

wil::com_ptr_nothrow<IContextMenu> ShellItemContextMenu::MaybeGetShellContextMenu(HWND hwnd) const
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = SHBindToObject(nullptr, m_directory.Raw(), nullptr, IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return nullptr;
	}

	std::vector<PCITEMID_CHILD> pidlItemsRaw;
	std::transform(m_items.begin(), m_items.end(), std::back_inserter(pidlItemsRaw),
		[](const PidlChild &pidl) { return pidl.Raw(); });

	wil::com_ptr_nothrow<IContextMenu> contextMenu;
	hr = GetUIObjectOf(shellFolder.get(), hwnd, static_cast<UINT>(pidlItemsRaw.size()),
		pidlItemsRaw.data(), IID_PPV_ARGS(&contextMenu));

	if (FAILED(hr))
	{
		return nullptr;
	}

	return contextMenu;
}
