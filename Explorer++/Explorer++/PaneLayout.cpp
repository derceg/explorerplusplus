// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PaneLayout.h"
#include <algorithm>

namespace
{

constexpr double MAXIMUM_PANE_SIZE_PERCENTAGE = 0.8;

int CalculatePaneSize(int preferredSize, int minimumSize, int availableSize)
{
	int maximumSize = static_cast<int>(MAXIMUM_PANE_SIZE_PERCENTAGE * std::max(availableSize, 0));
	// A very small window may not have enough room even for the DPI-scaled minimum.
	int effectiveMinimumSize = std::clamp(minimumSize, 0, maximumSize);
	return std::clamp(preferredSize, effectiveMinimumSize, maximumSize);
}

}

PaneSizes CalculatePaneSizes(const PaneLayoutSettings &settings)
{
	int treeViewWidth = CalculatePaneSize(settings.treeViewPreferredWidth,
		settings.treeViewMinimumWidth, settings.availableWidth);
	int remainingWidth = settings.availableWidth - (settings.showFolders ? treeViewWidth : 0);

	return { .treeViewWidth = treeViewWidth,
		.displayWindowWidth = CalculatePaneSize(settings.displayWindowPreferredWidth,
			settings.displayWindowMinimumWidth, remainingWidth),
		.displayWindowHeight = CalculatePaneSize(settings.displayWindowPreferredHeight,
			settings.displayWindowMinimumHeight, settings.availableHeight) };
}
