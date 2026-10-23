// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainRebar.h"
#include "AppServices.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "MainRebarView.h"
#include "PopupMenuView.h"
#include "RebarBand.h"
#include "RebarBandStorageData.h"
#include <wil/common.h>

MainRebar::MainRebar(MainRebarView *view, const std::vector<BandDefinition> &bandDefinitions,
	const std::vector<RebarBandStorageData> &storageData, BrowserWindow *browser,
	AppServices *appServices) :
	m_view(view),
	m_browser(browser),
	m_appServices(appServices)
{
	m_view->SetDelegate(this);

	auto *config = m_appServices->GetConfig();

	AddBands(bandDefinitions, storageData);

	m_view->LockBands(config->lockToolbars.get());
	m_connections.push_back(
		config->lockToolbars.addObserver(std::bind_front(&RebarView::LockBands, m_view)));
}

MainRebarView *MainRebar::GetView() const
{
	return m_view;
}

void MainRebar::AddBands(const std::vector<BandDefinition> &bandDefinitions,
	const std::vector<RebarBandStorageData> &storageData)
{
	auto sortedBandDefinitions = SortBandsUsingStorageData(bandDefinitions, storageData);

	for (const auto &bandDefinition : sortedBandDefinitions)
	{
		AddBand(bandDefinition, MaybeGetStorageDataForBand(bandDefinition.id, storageData));
	}
}

void MainRebar::AddBand(const BandDefinition &bandDefinition,
	const RebarBandStorageData *optStorageData)
{
	auto [itr, didInsert] = m_idToBandDefinitionMap.insert({ bandDefinition.id, bandDefinition });
	CHECK(didInsert);

	auto *bandController = bandDefinition.bandController;
	m_view->AddBand(bandDefinition.id, bandController->GetView(),
		bandController->CanShowOverflowMenu() ? RebarView::OverflowMenuPolicy::Enabled
											  : RebarView::OverflowMenuPolicy::Disabled);

	m_view->ShowBand(bandDefinition.id, bandDefinition.visibilityOption->get());
	m_connections.push_back(bandDefinition.visibilityOption->addObserver(
		std::bind_front(&RebarView::ShowBand, m_view, bandDefinition.id)));

	if (optStorageData)
	{
		m_view->SetBandWidth(bandDefinition.id, optStorageData->length);
		m_view->SetBandOnNewLine(bandDefinition.id,
			WI_IsFlagSet(optStorageData->style, RBBS_BREAK));
	}
}

std::vector<MainRebar::BandDefinition> MainRebar::SortBandsUsingStorageData(
	const std::vector<BandDefinition> &bandDefinitions,
	const std::vector<RebarBandStorageData> &storageData)
{
	auto sortedBandDefinitions = bandDefinitions;

	auto getSortedBandIndex = [&storageData](UINT bandId) -> size_t
	{
		auto itr = std::find_if(storageData.begin(), storageData.end(),
			[bandId](const auto &bandStorageData) { return bandStorageData.id == bandId; });

		if (itr == storageData.end())
		{
			// Any band that doesn't appear in the loaded data will be placed at the end.
			return storageData.size();
		}

		return itr - storageData.begin();
	};

	std::ranges::stable_sort(sortedBandDefinitions,
		[getSortedBandIndex](const auto &band1, const auto &band2)
		{ return getSortedBandIndex(band1.id) < getSortedBandIndex(band2.id); });

	return sortedBandDefinitions;
}

const RebarBandStorageData *MainRebar::MaybeGetStorageDataForBand(UINT bandId,
	const std::vector<RebarBandStorageData> &storageData)
{
	auto itr = std::find_if(storageData.begin(), storageData.end(),
		[bandId](const auto &bandStorageData) { return bandStorageData.id == bandId; });

	if (itr == storageData.end())
	{
		return nullptr;
	}

	return &*itr;
}

void MainRebar::OnShowContextMenu(UINT bandId, const POINT &ptScreen)
{
	const auto &bandDefinition = GetDefinitionForBand(bandId);

	PopupMenuView popupMenu(m_view->GetHWND(), m_browser);
	ToolbarOptionsMenu menu(&popupMenu, bandDefinition.contextMenuSource, m_browser, m_appServices);
	popupMenu.Show(ptScreen);
}

void MainRebar::OnShowOverflowMenu(UINT bandId, const POINT &ptScreen)
{
	const auto &bandDefinition = GetDefinitionForBand(bandId);
	auto *controller = bandDefinition.bandController;

	if (!controller->CanShowOverflowMenu())
	{
		return;
	}

	controller->ShowOverflowMenu(ptScreen);
}

const MainRebar::BandDefinition &MainRebar::GetDefinitionForBand(UINT bandId) const
{
	auto itr = m_idToBandDefinitionMap.find(bandId);
	CHECK(itr != m_idToBandDefinitionMap.end());
	return itr->second;
}
