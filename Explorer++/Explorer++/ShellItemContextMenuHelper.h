// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellItemContextMenu.h"

class AcceleratorManager;
class BrowserWindow;
class PidlAbsolute;
class ResourceLoader;

void ShowShellContextMenuForItem(const PidlAbsolute &pidl, HWND ownerWindow, const POINT &ptScreen,
	ShellItemContextMenu::Flags flags, BrowserWindow *browser,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader,
	ShellItemContextMenu::MaybeHandleShellMenuItemCallback maybeHandleShellMenuItemCallback = {},
	IUnknown *site = nullptr);
