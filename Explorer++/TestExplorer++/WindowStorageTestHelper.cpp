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
			.treeViewWidth = 430,
			.displayWindowWidth = 610,
			.displayWindowHeight = 429,
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
			.treeViewWidth = 57,
			.displayWindowWidth = 176,
			.displayWindowHeight = 827,
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
			.treeViewWidth = 844,
			.displayWindowWidth = 140,
			.displayWindowHeight = 211,
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
		.treeViewWidth = 117,
		.displayWindowWidth = 66,
		.displayWindowHeight = 595,
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
		.treeViewWidth = 1014,
		.displayWindowWidth = 212,
		.displayWindowHeight = 340,
	};
}

}
