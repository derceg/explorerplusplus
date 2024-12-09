// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellItemsMenu.h"

class HistoryModel;

// Displays the set of global history entries.
class HistoryMenu : public ShellItemsMenu
{
public:
	HistoryMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		HistoryModel *historyModel, BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader);
	HistoryMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		HistoryModel *historyModel, BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader,
		UINT menuStartId, UINT menuEndId);

private:
	void Initialize();
	void OnHistoryChanged();
	static std::vector<PidlAbsolute> GetHistoryItems(const HistoryModel *historyModel);

	HistoryModel *const m_historyModel;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
