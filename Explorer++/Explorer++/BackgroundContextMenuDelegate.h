// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellContextMenuDelegate.h"

class BrowserWindow;
class ResourceLoader;

// Adds a number of custom items to the background context menu.
class BackgroundContextMenuDelegate : public ShellContextMenuDelegate
{
public:
	BackgroundContextMenuDelegate(const BrowserWindow *browser,
		const ResourceLoader *resourceLoader);

	void UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		ShellContextMenuBuilder *builder) override;
	bool MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems, const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		UINT menuItemId) override;
	std::wstring GetHelpTextForCustomItem(UINT menuItemId) override;

private:
	void RemoveNonFunctionalItems(ShellContextMenuBuilder *builder);

	const BrowserWindow *const m_browser;
	const ResourceLoader *const m_resourceLoader;
};
