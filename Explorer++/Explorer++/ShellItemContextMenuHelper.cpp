// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellItemContextMenuHelper.h"
#include "BrowserWindow.h"
#include "PopupMenuView.h"
#include "../Helper/ShellHelper.h"

void ShowShellContextMenuForItem(const PidlAbsolute &pidl, HWND ownerWindow, const POINT &ptScreen,
	ShellItemContextMenu::Flags flags, BrowserWindow *browser,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader,
	ShellItemContextMenu::MaybeHandleShellMenuItemCallback maybeHandleShellMenuItemCallback,
	IUnknown *site)
{
	auto child = pidl.GetLastItem();

	// The root desktop pidl contains no items, so attempting to remove the last item will fail in
	// that case. Passing the root item as both the parent and the child works, however, and will
	// result in the context menu for the root item being shown.
	auto parentPidl = pidl;
	auto res = parentPidl.RemoveLastItem();
	CHECK(res || IsNamespaceRoot(pidl.Raw()));

	PopupMenuView popupMenu(ownerWindow, browser);
	ShellItemContextMenu menu(&popupMenu, acceleratorManager, parentPidl, { child }, site, flags,
		browser, resourceLoader, maybeHandleShellMenuItemCallback);
	popupMenu.Show(ptScreen);
}
