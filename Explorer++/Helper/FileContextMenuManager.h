// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Macros.h"
#include "PidlHelper.h"
#include "StatusBar.h"
#include <wil/com.h>
#include <optional>
#include <vector>

class FileContextMenuHandler
{
public:
	virtual ~FileContextMenuHandler() = default;

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

class FileContextMenuManager
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

	FileContextMenuManager(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PCITEMID_CHILD> &pidlItems, FileContextMenuHandler *handler,
		StatusBar *statusBar);

	void ShowMenu(HWND hwnd, const POINT *pt, IUnknown *site, Flags flags);

private:
	DISALLOW_COPY_AND_ASSIGN(FileContextMenuManager);

	wil::com_ptr_nothrow<IContextMenu> MaybeGetShellContextMenu(HWND hwnd) const;
	std::optional<std::string> MaybeGetFilesystemDirectory() const;

	LRESULT ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	const PidlAbsolute m_pidlParent;
	const std::vector<PidlChild> m_pidlItems;
	FileContextMenuHandler *const m_handler;
	StatusBar *const m_statusBar;
	wil::com_ptr_nothrow<IContextMenu> m_contextMenu;
};

DEFINE_ENUM_FLAG_OPERATORS(FileContextMenuManager::Flags);
