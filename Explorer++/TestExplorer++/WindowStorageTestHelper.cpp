// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "WindowStorageTestHelper.h"
#include "PidlTestHelper.h"
#include "RebarBandStorageData.h"
#include "TabStorage.h"
#include "TabStorageTestHelper.h"
#include "WindowStorage.h"

namespace WindowStorageTestHelper
{

std::vector<WindowStorageData> BuildV2ReferenceWindows(TestStorageType storageType)
{
	return {
		{
			.bounds = { 618, 598, 1825, 1249 },
			.showState = WindowShowState::Normal,
			.tabs = {
				CreateTabStorageFromDirectory(L"c:\\fake1", storageType),
			},
			.selectedTab = 0,
			.mainRebarInfo = {
				{ 1, 769, 212 },
				{ 0, 768, 391 },
			},
			.mainToolbarLayout = MainToolbarLayout({
				MainToolbarButtonId::Back,
				MainToolbarButtonId::Forward,
				MainToolbarButtonId::Delete,
			}),
			.treeViewPreferredWidth = 430,
			.displayWindowPreferredWidth = 610,
			.displayWindowPreferredHeight = 429,
		},
		{
			.bounds = { 212, 40, 400, 1073 },
			.showState = WindowShowState::Minimized,
			.tabs = {
				CreateTabStorageFromDirectory(L"c:\\fake1", storageType),
				CreateTabStorageFromDirectory(L"c:\\fake2", storageType),
				CreateTabStorageFromDirectory(L"c:\\fake3", storageType),
			},
			.selectedTab = 2,
			.mainRebarInfo = {
				{ 0, 769, 110 },
			},
			.mainToolbarLayout = MainToolbarLayout({
				MainToolbarButtonId::Refresh,
				MainToolbarButtonId::Separator,
				MainToolbarButtonId::NewTab,
			}),
			.treeViewPreferredWidth = 57,
			.displayWindowPreferredWidth = 176,
			.displayWindowPreferredHeight = 827,
		},
		{
			.bounds = { 1165, 2, 2071, 643 },
			.showState = WindowShowState::Maximized,
			.tabs = {
				CreateTabStorageFromDirectory(L"c:\\fake1", storageType),
				CreateTabStorageFromDirectory(L"c:\\fake2", storageType),
			},
			.selectedTab = 1,
			.mainRebarInfo = {
				{ 0, 769, 1846 },
			},
			.mainToolbarLayout = MainToolbarLayout({
				MainToolbarButtonId::Search,
				MainToolbarButtonId::CloseTab,
			}),
			.treeViewPreferredWidth = 844,
			.displayWindowPreferredWidth = 140,
			.displayWindowPreferredHeight = 211,
		}
	};
}

WindowStorageData BuildV2FallbackReferenceWindow(TestStorageType storageType)
{
	return {
		.bounds = { 683, 790, 1073, 2280 },
		.showState = WindowShowState::Normal,
		.tabs = {
			CreateTabStorageFromDirectory(L"c:\\fake1", storageType),
			CreateTabStorageFromDirectory(L"c:\\fake2", storageType),
			CreateTabStorageFromDirectory(L"c:\\fake3", storageType),
		},
		.selectedTab = 2,
		.mainRebarInfo = {
			{ 2, 769, 88 },
			{ 0, 768, 712 },
			{ 1, 768, 1834 },
		},
		.mainToolbarLayout = MainToolbarLayout({
			MainToolbarButtonId::MergeFiles,
			MainToolbarButtonId::OpenCommandPrompt,
		}),
		.treeViewPreferredWidth = 117,
		.displayWindowPreferredWidth = 66,
		.displayWindowPreferredHeight = 595,
	};
}

WindowStorageData BuildV1ReferenceWindow(TestStorageType storageType)
{
	return {
		.bounds = { 98, 87, 1606, 798 },
		.showState = WindowShowState::Normal,
		.tabs = {
			CreateTabStorageFromDirectory(L"c:\\fake1", storageType),
			CreateTabStorageFromDirectory(L"c:\\fake2", storageType),
		},
		.selectedTab = 1,
		.mainRebarInfo = {
			{ 0, 768, 652 },
			{ 1, 769, 839 },
		},
		.mainToolbarLayout = MainToolbarLayout({
			MainToolbarButtonId::CopyTo,
			MainToolbarButtonId::MoveTo,
			MainToolbarButtonId::Up,
		}),
		.treeViewPreferredWidth = 1014,
		.displayWindowPreferredWidth = 212,
		.displayWindowPreferredHeight = 340,
	};
}

}
