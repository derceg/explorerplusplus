// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellContextMenu.h"
#include <functional>
#include <string>
#include <vector>

class BrowserList;
class BrowserWindow;
class ResourceLoader;

// This class can be used to display a shell item context menu (i.e. the menu that's shown when
// right-clicking one or more items).
class ShellItemContextMenu : public ShellContextMenu
{
public:
	using MaybeHandleShellMenuItemCallback = std::function<bool(const PidlAbsolute &directory,
		const std::vector<PidlChild> &items, const std::wstring &verb)>;

	enum class Flags
	{
		None = 0,
		Rename = 1 << 0,
		ExtendedVerbs = 1 << 1
	};

	ShellItemContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const PidlAbsolute &directory, const std::vector<PidlChild> &items, HWND hwnd,
		IUnknown *site, Flags flags, BrowserWindow *browser, const ResourceLoader *resourceLoader,
		MaybeHandleShellMenuItemCallback maybeHandleShellMenuItemCallback = {});
	ShellItemContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const PidlAbsolute &directory, const std::vector<PidlChild> &items, HWND hwnd,
		IUnknown *site, Flags flags, BrowserList *browserList, const ResourceLoader *resourceLoader,
		MaybeHandleShellMenuItemCallback maybeHandleShellMenuItemCallback = {});

private:
	ShellItemContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const PidlAbsolute &directory, const std::vector<PidlChild> &items, HWND hwnd,
		IUnknown *site, Flags flags, BrowserWindow *browser, BrowserList *browserList,
		const ResourceLoader *resourceLoader,
		MaybeHandleShellMenuItemCallback maybeHandleShellMenuItemCallback);

	void BuildMenu(IUnknown *site, Flags flags, const ResourceLoader *resourceLoader);
	void MaybeAddOpenInNewTabItem(const ResourceLoader *resourceLoader);

	// ShellContextMenu
	wil::com_ptr_nothrow<IContextMenu> MaybeGetShellContextMenu(HWND hwnd) const override;
	bool MaybeHandleShellMenuItem(const std::wstring &verb) override;

	void OnOpenDirectoryInNewTab(const PidlAbsolute &directory);
	BrowserWindow *GetTargetBrowser() const;

	const std::vector<PidlChild> m_items;
	BrowserWindow *const m_browser;
	BrowserList *const m_browserList;
	MaybeHandleShellMenuItemCallback m_maybeHandleShellMenuItemCallback;
};

DEFINE_ENUM_FLAG_OPERATORS(ShellItemContextMenu::Flags);
