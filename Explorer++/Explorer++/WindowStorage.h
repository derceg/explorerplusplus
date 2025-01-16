// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "LayoutDefaults.h"
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
	// The size and position of the window, with the position being in workspace coordinates (i.e.
	// those returned by GetWindowPlacement()).
	RECT bounds = LayoutDefaults::GetDefaultMainWindowBounds();

	WindowShowState showState = WindowShowState::Normal;
	std::vector<TabStorageData> tabs;
	int selectedTab = 0;
	std::vector<RebarBandStorageInfo> mainRebarInfo;
	std::optional<MainToolbarStorage::MainToolbarButtons> mainToolbarButtons;
	int treeViewWidth = LayoutDefaults::DEFAULT_TREEVIEW_WIDTH;
	int displayWindowWidth = LayoutDefaults::DEFAULT_DISPLAY_WINDOW_WIDTH;
	int displayWindowHeight = LayoutDefaults::DEFAULT_DISPLAY_WINDOW_HEIGHT;

	// This is only used in tests.
	bool operator==(const WindowStorageData &other) const;
};

WindowShowState NativeShowStateToShowState(int nativeShowState);
int ShowStateToNativeShowState(WindowShowState showState);
