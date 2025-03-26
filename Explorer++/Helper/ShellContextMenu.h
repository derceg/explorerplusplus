// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PidlHelper.h"
#include <boost/core/noncopyable.hpp>
#include <wil/com.h>
#include <optional>
#include <vector>

class MenuHelpTextRequest;

class ShellContextMenuHandler
{
public:
	virtual ~ShellContextMenuHandler() = default;

	// Allows the caller to add/update items on the context menu before it's shown.
	virtual void UpdateMenuEntries(HMENU menu, PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems, IContextMenu *contextMenu) = 0;

	// Retrieves the help text for a custom menu item.
	virtual std::wstring GetHelpTextForItem(UINT menuItemId) = 0;

	// Allows the caller to handle the processing of a shell menu item. For example, the 'Open' item
	// may be processed internally.
	// Returns true if the item was processed; false otherwise.
	virtual bool HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems, const std::wstring &verb) = 0;

	// Handles the processing for one of the menu items that was added by the caller.
	virtual void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems, UINT menuItemId) = 0;
};

class ShellContextMenu : private boost::noncopyable
{
public:
	enum class Flags
	{
		Standard = 0,
		Rename = 1 << 0,
		ExtendedVerbs = 1 << 1
	};

	static const int MIN_SHELL_MENU_ID = 1;
	static const int MAX_SHELL_MENU_ID = 1000;

	ShellContextMenu(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PCITEMID_CHILD> &pidlItems,
		ShellContextMenuHandler *handler, MenuHelpTextRequest *menuHelpTextRequest);

	void ShowMenu(HWND hwnd, const POINT *pt, IUnknown *site, Flags flags);

private:
	wil::com_ptr_nothrow<IContextMenu> MaybeGetShellContextMenu(HWND hwnd) const;
	std::optional<std::string> MaybeGetFilesystemDirectory() const;

	LRESULT ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	std::optional<std::wstring> MaybeGetMenuHelpText(HMENU shellContextMenu, HMENU menu, int id);

	const PidlAbsolute m_pidlParent;
	const std::vector<PidlChild> m_pidlItems;
	ShellContextMenuHandler *const m_handler;
	MenuHelpTextRequest *const m_menuHelpTextRequest;
	wil::com_ptr_nothrow<IContextMenu> m_contextMenu;
};

DEFINE_ENUM_FLAG_OPERATORS(ShellContextMenu::Flags);
