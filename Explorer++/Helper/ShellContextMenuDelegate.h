// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PidlHelper.h"
#include <string>
#include <vector>

class ShellContextMenuBuilder;

class ShellContextMenuDelegate
{
public:
	virtual ~ShellContextMenuDelegate() = default;

	// Allows the delegate to add/remove items on the context menu before it's shown.
	virtual void UpdateMenuEntries(PCIDLIST_ABSOLUTE directory, const std::vector<PidlChild> &items,
		ShellContextMenuBuilder *builder) = 0;

	// Allows the delegate to handle the processing of a shell menu item. For example, the 'Open'
	// item may be processed internally.
	//
	// Should return true if the item was processed; false otherwise.
	virtual bool MaybeHandleShellMenuItem(PCIDLIST_ABSOLUTE directory,
		const std::vector<PidlChild> &items, const std::wstring &verb) = 0;

	// Handles the processing for one of the menu items that was added by the delegate.
	virtual void HandleCustomMenuItem(PCIDLIST_ABSOLUTE directory,
		const std::vector<PidlChild> &items, UINT menuItemId) = 0;

	// Retrieves the help text for a custom menu item.
	virtual std::wstring GetHelpTextForCustomItem(UINT menuItemId) = 0;
};
