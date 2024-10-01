// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GlobalHistoryMenu.h"
#include "HistoryService.h"

GlobalHistoryMenu::GlobalHistoryMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, HistoryService *historyService,
	BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader) :
	ShellItemsMenu(menuView, acceleratorManager, GetHistoryItems(historyService), browserWindow,
		shellIconLoader),
	m_historyService(historyService)
{
	Initialize();
}

GlobalHistoryMenu::GlobalHistoryMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, HistoryService *historyService,
	BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader, UINT menuStartId,
	UINT menuEndId) :
	ShellItemsMenu(menuView, acceleratorManager, GetHistoryItems(historyService), browserWindow,
		shellIconLoader, menuStartId, menuEndId),
	m_historyService(historyService)
{
	Initialize();
}

void GlobalHistoryMenu::Initialize()
{
	m_connections.push_back(m_historyService->AddHistoryChangedObserver(
		std::bind_front(&GlobalHistoryMenu::OnHistoryChanged, this)));
}

void GlobalHistoryMenu::OnHistoryChanged()
{
	RebuildMenu(GetHistoryItems(m_historyService));
}

std::vector<PidlAbsolute> GlobalHistoryMenu::GetHistoryItems(const HistoryService *historyService)
{
	const auto &history = historyService->GetHistoryItems();
	std::vector<PidlAbsolute> historyVector({ history.begin(), history.end() });
	return historyVector;
}
