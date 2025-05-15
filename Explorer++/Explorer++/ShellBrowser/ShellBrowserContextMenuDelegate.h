// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellItemContextMenuDelegate.h"
#include "../Helper/WeakPtr.h"

class ShellBrowserImpl;

// When the shell context menu is being shown for one or more items in a ShellBrowserImpl instance,
// this class handles the view-specific items (e.g. the "rename" item is view-specific, since it
// acts on the view).
class ShellBrowserContextMenuDelegate : public ShellItemContextMenuDelegate
{
public:
	ShellBrowserContextMenuDelegate(WeakPtr<ShellBrowserImpl> shellBrowserWeak);

	void UpdateMenuEntries(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		ShellContextMenuBuilder *builder) override;
	bool MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		UINT menuItemId) override;
	std::wstring GetHelpTextForCustomItem(UINT menuItemId) override;

private:
	std::vector<PidlAbsolute> BuildItemList(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems) const;

	const WeakPtr<ShellBrowserImpl> m_shellBrowserWeak;
};
