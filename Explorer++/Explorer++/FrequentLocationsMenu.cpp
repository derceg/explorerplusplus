// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FrequentLocationsMenu.h"
#include "FrequentLocationsModel.h"
#include <ranges>

FrequentLocationsMenu::FrequentLocationsMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, FrequentLocationsModel *frequentLocationsModel,
	BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader, UINT startId, UINT endId) :
	ShellItemsMenu(menuView, acceleratorManager, GetLocations(frequentLocationsModel),
		browserWindow, shellIconLoader, startId, endId),
	m_frequentLocationsModel(frequentLocationsModel)
{
	m_connections.push_back(m_frequentLocationsModel->AddLocationsChangedObserver(
		std::bind_front(&FrequentLocationsMenu::OnLocationsChanged, this)));
}

void FrequentLocationsMenu::OnLocationsChanged()
{
	RebuildMenu(GetLocations(m_frequentLocationsModel));
}

std::vector<PidlAbsolute> FrequentLocationsMenu::GetLocations(
	const FrequentLocationsModel *frequentLocationsModel)
{
	std::vector<PidlAbsolute> pidls;

	for (const auto &visit : frequentLocationsModel->GetVisits() | std::views::take(MAX_MENU_ITEMS))
	{
		pidls.push_back(visit.GetLocation());
	}

	return pidls;
}
