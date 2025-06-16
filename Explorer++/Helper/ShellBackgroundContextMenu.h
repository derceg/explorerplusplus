// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellContextMenu.h"

class ShellBackgroundContextMenuDelegate;

// Can be used to display a background context menu (i.e. the menu that's shown when right-clicking
// the directory view background).
class ShellBackgroundContextMenu : public ShellContextMenu
{
public:
	enum class Flags
	{
		None = 0,
		ExtendedVerbs = 1 << 0
	};

	ShellBackgroundContextMenu(PCIDLIST_ABSOLUTE directory, MenuHelpTextHost *menuHelpTextHost);

	void AddDelegate(ShellBackgroundContextMenuDelegate *delegate);
	void ShowMenu(HWND hwnd, const POINT *pt, IUnknown *site, Flags flags);

protected:
	wil::com_ptr_nothrow<IContextMenu> MaybeGetShellContextMenu(HWND hwnd) const override;
};

DEFINE_ENUM_FLAG_OPERATORS(ShellBackgroundContextMenu::Flags);
