// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GlobalHistoryMenu.h"
#include "HistoryModel.h"

GlobalHistoryMenu::GlobalHistoryMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, HistoryModel *historyModel,
	BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader) :
	ShellItemsMenu(menuView, acceleratorManager, GetHistoryItems(historyModel), browserWindow,
		shellIconLoader),
	m_historyModel(historyModel)
{
	Initialize();
}

GlobalHistoryMenu::GlobalHistoryMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, HistoryModel *historyModel,
	BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader, UINT menuStartId,
	UINT menuEndId) :
	ShellItemsMenu(menuView, acceleratorManager, GetHistoryItems(historyModel), browserWindow,
		shellIconLoader, menuStartId, menuEndId),
	m_historyModel(historyModel)
{
	Initialize();
}

void GlobalHistoryMenu::Initialize()
{
	m_connections.push_back(m_historyModel->AddHistoryChangedObserver(
		std::bind_front(&GlobalHistoryMenu::OnHistoryChanged, this)));
}

void GlobalHistoryMenu::OnHistoryChanged()
{
	RebuildMenu(GetHistoryItems(m_historyModel));
}

std::vector<PidlAbsolute> GlobalHistoryMenu::GetHistoryItems(const HistoryModel *historyModel)
{
	const auto &history = historyModel->GetHistoryItems();
	std::vector<PidlAbsolute> historyVector({ history.begin(), history.end() });
	return historyVector;
}
