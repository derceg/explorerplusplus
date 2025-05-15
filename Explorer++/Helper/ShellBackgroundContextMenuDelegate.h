// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellContextMenuDelegate.h"

// Delegates that inherit from this can only be used with the background context menu (i.e. the menu
// that's shown when right-clicking the directory view background).
class ShellBackgroundContextMenuDelegate : public ShellContextMenuDelegate
{
public:
	void UpdateMenuEntries(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		ShellContextMenuBuilder *builder) final override;
	bool MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		const std::wstring &verb) final override;
	void HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		UINT menuItemId) final override;

protected:
	virtual void UpdateMenuEntries(PCIDLIST_ABSOLUTE directory,
		ShellContextMenuBuilder *builder) = 0;
	virtual bool MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory,
		const std::wstring &verb) = 0;
	virtual void HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory, UINT menuItemId) = 0;
};
