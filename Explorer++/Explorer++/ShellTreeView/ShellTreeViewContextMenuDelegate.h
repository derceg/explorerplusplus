// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellContextMenuDelegate.h"

class ShellTreeView;

// When the shell context menu is being shown for an item in a ShellTreeView instance, this class
// handles the view-specific items.
class ShellTreeViewContextMenuDelegate : public ShellContextMenuDelegate
{
public:
	ShellTreeViewContextMenuDelegate(ShellTreeView *shellTreeView);

	void UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		ShellContextMenuBuilder *builder) override;
	bool MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems, const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		UINT menuItemId) override;
	std::wstring GetHelpTextForCustomItem(UINT menuItemId) override;

private:
	ShellTreeView *const m_shellTreeView;
};
