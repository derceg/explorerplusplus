// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBackgroundContextMenu.h"
#include "ShellBackgroundContextMenuDelegate.h"
#include <wil/common.h>

ShellBackgroundContextMenu::ShellBackgroundContextMenu(PCIDLIST_ABSOLUTE directory,
	MenuHelpTextRequest *menuHelpTextRequest) :
	ShellContextMenu(directory, {}, menuHelpTextRequest)
{
}

void ShellBackgroundContextMenu::AddDelegate(ShellBackgroundContextMenuDelegate *delegate)
{
	ShellContextMenu::AddDelegate(delegate);
}

void ShellBackgroundContextMenu::ShowMenu(HWND hwnd, const POINT *pt, IUnknown *site, Flags flags)
{
	UINT contextMenuflags = CMF_NODEFAULT;

	if (WI_IsFlagSet(flags, Flags::ExtendedVerbs))
	{
		contextMenuflags |= CMF_EXTENDEDVERBS;
	}

	ShellContextMenu::ShowMenu(hwnd, pt, site, contextMenuflags);
}

wil::com_ptr_nothrow<IContextMenu> ShellBackgroundContextMenu::MaybeGetShellContextMenu(
	HWND hwnd) const
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = SHBindToObject(nullptr, m_directory.Raw(), nullptr, IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<IContextMenu> contextMenu;
	hr = shellFolder->CreateViewObject(hwnd, IID_PPV_ARGS(&contextMenu));

	if (FAILED(hr))
	{
		return nullptr;
	}

	return contextMenu;
}
