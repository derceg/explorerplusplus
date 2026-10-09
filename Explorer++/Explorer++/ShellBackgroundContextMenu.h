// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellContextMenu.h"
#include <memory>
#include <vector>

class BrowserWindow;
class ClipboardStore;
enum class PasteType;
class ResourceLoader;

// This class can be used to display a background context menu (i.e. the menu that's shown when
// right-clicking the directory view background).
class ShellBackgroundContextMenu : public ShellContextMenu
{
public:
	enum class Flags
	{
		None = 0,
		ExtendedVerbs = 1 << 0
	};

	ShellBackgroundContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const PidlAbsolute &directory, HWND hwnd, IUnknown *site, Flags flags,
		BrowserWindow *browser, ClipboardStore *clipboardStore,
		const ResourceLoader *resourceLoader);

private:
	void BuildMenu(IUnknown *site, Flags flags, const ResourceLoader *resourceLoader);
	bool CanPaste(PasteType pasteType) const;

	// ShellContextMenu
	wil::com_ptr_nothrow<IContextMenu> MaybeGetShellContextMenu(HWND hwnd) const override;

	void OnCustomizeDirectory();
	void SendCommandToBrowser(WORD id);

	BrowserWindow *const m_browser;
	ClipboardStore *const m_clipboardStore;
	std::vector<std::unique_ptr<MenuBase>> m_childMenus;
};

DEFINE_ENUM_FLAG_OPERATORS(ShellBackgroundContextMenu::Flags);
