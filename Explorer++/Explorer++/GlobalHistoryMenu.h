// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellItemsMenu.h"

class HistoryService;

// Displays the set of global history entries.
class GlobalHistoryMenu : public ShellItemsMenu
{
public:
	GlobalHistoryMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		HistoryService *historyService, BrowserWindow *browserWindow, IconFetcher *iconFetcher);
	GlobalHistoryMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		HistoryService *historyService, BrowserWindow *browserWindow, IconFetcher *iconFetcher,
		UINT menuStartId, UINT menuEndId);

private:
	void Initialize();
	void OnHistoryChanged();
	static std::vector<PidlAbsolute> GetHistoryItems(const HistoryService *historyService);

	HistoryService *const m_historyService;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
