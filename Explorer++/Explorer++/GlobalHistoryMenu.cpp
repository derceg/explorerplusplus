// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GlobalHistoryMenu.h"
#include "HistoryService.h"

GlobalHistoryMenu::GlobalHistoryMenu(MenuView *menuView, HistoryService *historyService,
	BrowserWindow *browserWindow, IconFetcher *iconFetcher) :
	ShellItemsMenu(menuView, GetHistoryItems(historyService), browserWindow, iconFetcher),
	m_historyService(historyService)
{
	Initialize();
}

GlobalHistoryMenu::GlobalHistoryMenu(MenuView *menuView, HistoryService *historyService,
	BrowserWindow *browserWindow, IconFetcher *iconFetcher, UINT menuStartId, UINT menuEndId) :
	ShellItemsMenu(menuView, GetHistoryItems(historyService), browserWindow, iconFetcher,
		menuStartId, menuEndId),
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

std::vector<PidlAbsolute> GlobalHistoryMenu::GetHistoryItems(HistoryService *historyService)
{
	const auto &history = historyService->GetHistoryItems();
	std::vector<PidlAbsolute> historyVector({ history.begin(), history.end() });
	return historyVector;
}
