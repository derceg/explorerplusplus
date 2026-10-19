// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RebarViewDelegate.h"
#include "ToolbarOptionsMenu.h"
#include "ValueWrapper.h"
#include <boost/signals2.hpp>
#include <unordered_map>
#include <vector>

class AppServices;
class BrowserWindow;
class MainRebarView;
class RebarBand;
struct RebarBandStorageData;

class MainRebar : private RebarViewDelegate
{
public:
	struct BandDefinition
	{
		UINT id;
		RebarBand *bandController = nullptr;
		ValueWrapper<bool> *visibilityOption = nullptr;
		ToolbarOptionsMenu::Source contextMenuSource;
	};

	static MainRebar *Create(MainRebarView *view,
		const std::vector<BandDefinition> &bandDefinitions,
		const std::vector<RebarBandStorageData> &storageData, BrowserWindow *browser,
		AppServices *appServices);

	MainRebarView *GetView() const;

private:
	MainRebar(MainRebarView *view, const std::vector<BandDefinition> &bandDefinitions,
		const std::vector<RebarBandStorageData> &storageData, BrowserWindow *browser,
		AppServices *appServices);

	void AddBands(const std::vector<BandDefinition> &bandDefinitions,
		const std::vector<RebarBandStorageData> &storageData);
	void AddBand(const BandDefinition &bandDefinition, const RebarBandStorageData *optStorageData);
	static std::vector<BandDefinition> SortBandsUsingStorageData(
		const std::vector<BandDefinition> &bandDefinitions,
		const std::vector<RebarBandStorageData> &storageData);
	static const RebarBandStorageData *MaybeGetStorageDataForBand(UINT bandId,
		const std::vector<RebarBandStorageData> &storageData);

	// RebarViewDelegate
	void OnShowContextMenu(UINT bandId, const POINT &ptScreen) override;
	void OnShowOverflowMenu(UINT bandId, const POINT &ptScreen) override;

	const BandDefinition &GetDefinitionForBand(UINT bandId) const;

	void OnWindowDestroyed();

	MainRebarView *const m_view;
	BrowserWindow *const m_browser;
	AppServices *const m_appServices;
	std::unordered_map<UINT, BandDefinition> m_idToBandDefinitionMap;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
