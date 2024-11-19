// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "WindowStorageTestHelper.h"
#include "MainRebarStorage.h"
#include "ShellTestHelper.h"
#include "TabStorage.h"
#include "TabStorageTestHelper.h"
#include "WindowStorage.h"

namespace WindowStorageTestHelper
{

std::vector<WindowStorageData> BuildV2ReferenceWindows(TestStorageType storageType)
{
	// clang-format off
	return {
		{
			{ 618, 598, 1825, 1249 },
			WindowShowState::Normal,
			{
				CreateTabStorageFromDirectory(L"c:\\fake1", storageType)
			},
			0,
			{
				{ 1, 769, 212 },
				{ 0, 768, 391 }
			},
			MainToolbarStorage::MainToolbarButtons({
				MainToolbarButton::Back,
				MainToolbarButton::Forward,
				MainToolbarButton::Delete
			})
		},
		{
			{ 212, 40, 400, 1073 },
			WindowShowState::Minimized,
			{
				CreateTabStorageFromDirectory(L"c:\\fake1", storageType),
				CreateTabStorageFromDirectory(L"c:\\fake2", storageType),
				CreateTabStorageFromDirectory(L"c:\\fake3", storageType)
			},
			2,
			{
				{ 0, 769, 110 }
			},
			MainToolbarStorage::MainToolbarButtons({
				MainToolbarButton::Refresh,
				MainToolbarButton::Separator,
				MainToolbarButton::NewTab
			})
		},
		{
			{ 1165, 2, 2071, 643 },
			WindowShowState::Maximized,
			{
				CreateTabStorageFromDirectory(L"c:\\fake1", storageType),
				CreateTabStorageFromDirectory(L"c:\\fake2", storageType)
			},
			1,
			{
				{ 0, 769, 1846 }
			},
			MainToolbarStorage::MainToolbarButtons({
				MainToolbarButton::Search,
				MainToolbarButton::CloseTab
			})
		}
	};
	// clang-format on
}

WindowStorageData BuildV2FallbackReferenceWindow(TestStorageType storageType)
{
	// clang-format off
	return {
		{ 683, 790, 1073, 2280 },
		WindowShowState::Normal,
		{
			CreateTabStorageFromDirectory(L"c:\\fake1", storageType),
			CreateTabStorageFromDirectory(L"c:\\fake2", storageType),
			CreateTabStorageFromDirectory(L"c:\\fake3", storageType)
		},
		2,
		{
			{ 2, 769, 88 },
			{ 0, 768, 712 },
			{ 1, 768, 1834 }
		},
		MainToolbarStorage::MainToolbarButtons({
			MainToolbarButton::MergeFiles,
			MainToolbarButton::OpenCommandPrompt
		})
	};
	// clang-format on
}

WindowStorageData BuildV1ReferenceWindow(TestStorageType storageType)
{
	// clang-format off
	return {
		{ 98, 87, 1606, 798 },
		WindowShowState::Normal,
		{
			CreateTabStorageFromDirectory(L"c:\\fake1", storageType),
			CreateTabStorageFromDirectory(L"c:\\fake2", storageType)
		},
		1,
		{
			{ 0, 768, 652 },
			{ 1, 769, 839 }
		},
		MainToolbarStorage::MainToolbarButtons({
			MainToolbarButton::CopyTo,
			MainToolbarButton::MoveTo,
			MainToolbarButton::Up
		})
	};
	// clang-format on
}

}
