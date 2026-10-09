// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellItemContextMenu.h"

class SearchResultsItemContextMenu : public ShellItemContextMenu
{
public:
	SearchResultsItemContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		const PidlAbsolute &directory, const std::vector<PidlChild> &items, HWND hwnd,
		IUnknown *site, Flags flags, BrowserList *browserList,
		const ResourceLoader *resourceLoader);

private:
	void MaybeAddOpenLocationItem(const PidlAbsolute &directory,
		const std::vector<PidlChild> &items, const ResourceLoader *resourceLoader);
	void OnOpenLocation(const PidlAbsolute &directory);

	BrowserList *const m_browserList;
};
