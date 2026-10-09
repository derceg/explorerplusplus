// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "MenuView.h"
#include "../Helper/Pidl.h"
#include <wil/com.h>
#include <ShObjIdl.h>
#include <optional>
#include <string>
#include <vector>

// This class can be used to display a shell context menu (either an item context menu or background
// context menu). This is simply a base class, so exactly which menu is shown is up to the derived
// class.
class ShellContextMenu : public MenuBase, private ExternalMenuItemDelegate
{
public:
	ShellContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const PidlAbsolute &directory, HWND hwnd);

protected:
	void AppendShellItems(IUnknown *site, UINT flags);
	std::optional<UINT> MaybeGetShellItemIndex(const std::wstring &verb) const;
	void RemoveShellItems(const std::vector<std::wstring> &verbsToRemove);
	void RemoveRedundantSeparators();

	const PidlAbsolute m_directory;

private:
	// It's possible for a folder to not provide any IContextMenu instance (for example, the Home
	// folder in Windows 11 doesn't provide any IContextMenu instance for the background menu). So,
	// this method may return null.
	virtual wil::com_ptr_nothrow<IContextMenu> MaybeGetShellContextMenu(HWND hwnd) const = 0;

	void RemoveShellItem(const std::wstring &verb);

	// ExternalMenuItemDelegate
	std::optional<LRESULT> MaybeHandleMessage(UINT msg, WPARAM wParam,
		LPARAM lParam) override final;
	std::optional<std::wstring> MaybeGetExternalItemHelpText(UINT id) const override final;
	void OnExternalItemSelected(UINT id) override final;

	virtual bool MaybeHandleShellMenuItem(const std::wstring &verb);
	std::optional<std::wstring> MaybeGetFilesystemDirectory() const;

	const HWND m_hwnd;
	wil::com_ptr_nothrow<IContextMenu> m_contextMenu;
	wil::unique_set_site_null_call m_resetSite;
};
