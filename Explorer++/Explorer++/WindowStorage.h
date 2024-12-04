// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MainToolbarStorage.h"
#include "../Helper/BetterEnumsWrapper.h"
#include <optional>
#include <vector>

struct RebarBandStorageInfo;
struct TabStorageData;

// These values are used when loading and saving data and shouldn't be changed.
// clang-format off
BETTER_ENUM(WindowShowState, int,
	Normal = 0,
	Minimized = 1,
	Maximized = 2
)
// clang-format on

struct WindowStorageData
{
	RECT bounds;
	WindowShowState showState;
	std::vector<TabStorageData> tabs;
	int selectedTab;
	std::vector<RebarBandStorageInfo> mainRebarInfo;
	std::optional<MainToolbarStorage::MainToolbarButtons> mainToolbarButtons;
	int treeViewWidth;
	int displayWindowWidth;
	int displayWindowHeight;

	// This is only used in tests.
	bool operator==(const WindowStorageData &other) const;
};

WindowShowState NativeShowStateToShowState(int nativeShowState);
int ShowStateToNativeShowState(WindowShowState showState);
