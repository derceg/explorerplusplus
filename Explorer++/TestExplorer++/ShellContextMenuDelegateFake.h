// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellContextMenuDelegate.h"

class ShellContextMenuDelegateFake : public ShellContextMenuDelegate
{
public:
	void UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		ShellContextMenuBuilder *builder) override;
	bool MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
		const std::vector<PidlChild> &pidlItems, const std::wstring &verb) override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PidlChild> &pidlItems,
		UINT menuItemId) override;
	std::wstring GetHelpTextForCustomItem(UINT menuItemId) override;
};
