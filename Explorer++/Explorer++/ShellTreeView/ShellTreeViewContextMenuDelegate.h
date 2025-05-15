// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellItemContextMenuDelegate.h"

class ShellTreeView;

// When the shell context menu is being shown for an item in a ShellTreeView instance, this class
// handles the view-specific items.
class ShellTreeViewContextMenuDelegate : public ShellItemContextMenuDelegate
{
public:
	ShellTreeViewContextMenuDelegate(ShellTreeView *shellTreeView);

	void UpdateMenuEntries(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		ShellContextMenuBuilder *builder) override;
	bool MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		UINT menuItemId) override;
	std::wstring GetHelpTextForCustomItem(UINT menuItemId) override;

private:
	ShellTreeView *const m_shellTreeView;
};
