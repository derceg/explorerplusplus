// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PidlHelper.h"
#include "ShellContextMenuIdGenerator.h"
#include <boost/core/noncopyable.hpp>
#include <wil/com.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class MenuHelpTextHost;
class ShellContextMenuDelegate;
class ShellContextMenuIdRemapper;

// Can be used to display a shell context menu (either an item context menu or background context
// menu). This class is simply a base class, so exactly which menu is shown is up to the derived
// class.
class ShellContextMenu : private boost::noncopyable
{
public:
	static constexpr int MIN_SHELL_MENU_ID = 1;
	static constexpr int MAX_SHELL_MENU_ID = 1000;

	virtual ~ShellContextMenu();

protected:
	ShellContextMenu(PCIDLIST_ABSOLUTE directory, const std::vector<PCITEMID_CHILD> &items,
		MenuHelpTextHost *menuHelpTextHost);

	void AddDelegate(ShellContextMenuDelegate *delegate);
	void ShowMenu(HWND hwnd, const POINT *pt, IUnknown *site, UINT flags);

	// It's possible for a folder to not provide any IContextMenu instance (for example, the Home
	// folder in Windows 11 doesn't provide any IContextMenu instance for the background menu). So,
	// this method may return null.
	virtual wil::com_ptr_nothrow<IContextMenu> MaybeGetShellContextMenu(HWND hwnd) const = 0;

	const PidlAbsolute m_directory;
	const std::vector<PidlChild> m_items;

private:
	void UpdateMenuEntries(HMENU menu);
	bool MaybeHandleShellMenuItem(const std::wstring &verb);
	std::optional<std::string> MaybeGetFilesystemDirectory() const;

	LRESULT ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	std::optional<std::wstring> MaybeGetMenuHelpText(HMENU shellContextMenu, HMENU menu, int id);

	ShellContextMenuIdRemapper *GetIdRemapperForDelegate(ShellContextMenuDelegate *delegate);

	MenuHelpTextHost *const m_menuHelpTextHost;
	ShellContextMenuIdGenerator m_idGenerator;
	std::vector<ShellContextMenuDelegate *> m_delegates;
	std::unordered_map<ShellContextMenuDelegate *, std::unique_ptr<ShellContextMenuIdRemapper>>
		m_delegateToIdRemapperMap;
	wil::com_ptr_nothrow<IContextMenu> m_contextMenu;
};
