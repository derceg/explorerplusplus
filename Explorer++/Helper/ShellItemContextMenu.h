// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellContextMenu.h"

class ShellItemContextMenuDelegate;

// Can be used to display an item context menu (i.e. the menu that's shown when right-clicking one
// or more items).
class ShellItemContextMenu : public ShellContextMenu
{
public:
	enum class Flags
	{
		None = 0,
		Rename = 1 << 0,
		ExtendedVerbs = 1 << 1
	};

	ShellItemContextMenu(PCIDLIST_ABSOLUTE directory, const std::vector<PCITEMID_CHILD> &items,
		MenuHelpTextHost *menuHelpTextHost);

	void AddDelegate(ShellItemContextMenuDelegate *delegate);
	void ShowMenu(HWND hwnd, const POINT *pt, IUnknown *site, Flags flags);

protected:
	wil::com_ptr_nothrow<IContextMenu> MaybeGetShellContextMenu(HWND hwnd) const override;
};

DEFINE_ENUM_FLAG_OPERATORS(ShellItemContextMenu::Flags);
