// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellBackgroundContextMenuDelegate.h"

class BrowserWindow;
class ClipboardStore;
class ResourceLoader;

// Adds a number of custom items to the background context menu.
class BackgroundContextMenuDelegate : public ShellBackgroundContextMenuDelegate
{
public:
	BackgroundContextMenuDelegate(const BrowserWindow *browser, ClipboardStore *clipboardStore,
		const ResourceLoader *resourceLoader);

	void UpdateMenuEntries(PCIDLIST_ABSOLUTE directory, ShellContextMenuBuilder *builder) override;
	bool MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory, const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory, UINT menuItemId) override;
	std::wstring GetHelpTextForCustomItem(UINT menuItemId) override;

private:
	void RemoveNonFunctionalItems(ShellContextMenuBuilder *builder);

	const BrowserWindow *const m_browser;
	ClipboardStore *const m_clipboardStore;
	const ResourceLoader *const m_resourceLoader;
};
