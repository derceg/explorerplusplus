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

class MenuHelpTextRequest;
class ShellContextMenuDelegate;
class ShellContextMenuIdRemapper;

class ShellContextMenu : private boost::noncopyable
{
public:
	enum class Flags
	{
		Standard = 0,
		Rename = 1 << 0,
		ExtendedVerbs = 1 << 1
	};

	static constexpr int MIN_SHELL_MENU_ID = 1;
	static constexpr int MAX_SHELL_MENU_ID = 1000;

	ShellContextMenu(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PCITEMID_CHILD> &pidlItems,
		MenuHelpTextRequest *menuHelpTextRequest);
	~ShellContextMenu();

	void AddDelegate(ShellContextMenuDelegate *delegate);
	void ShowMenu(HWND hwnd, const POINT *pt, IUnknown *site, Flags flags);

private:
	wil::com_ptr_nothrow<IContextMenu> MaybeGetShellContextMenu(HWND hwnd) const;
	void UpdateMenuEntries(HMENU menu);
	bool MaybeHandleShellMenuItem(const std::wstring &verb);
	std::optional<std::string> MaybeGetFilesystemDirectory() const;

	LRESULT ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	std::optional<std::wstring> MaybeGetMenuHelpText(HMENU shellContextMenu, HMENU menu, int id);

	ShellContextMenuIdRemapper *GetIdRemapperForDelegate(ShellContextMenuDelegate *delegate);

	const PidlAbsolute m_pidlParent;
	const std::vector<PidlChild> m_pidlItems;
	MenuHelpTextRequest *const m_menuHelpTextRequest;
	ShellContextMenuIdGenerator m_idGenerator;
	std::vector<ShellContextMenuDelegate *> m_delegates;
	std::unordered_map<ShellContextMenuDelegate *, std::unique_ptr<ShellContextMenuIdRemapper>>
		m_delegateToIdRemapperMap;
	wil::com_ptr_nothrow<IContextMenu> m_contextMenu;
};

DEFINE_ENUM_FLAG_OPERATORS(ShellContextMenu::Flags);
