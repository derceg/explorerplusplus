// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct PaneLayoutSettings
{
	int availableWidth;
	int availableHeight;
	int treeViewPreferredWidth;
	int treeViewMinimumWidth;
	int displayWindowPreferredWidth;
	int displayWindowMinimumWidth;
	int displayWindowPreferredHeight;
	int displayWindowMinimumHeight;
	bool showFolders;
};

struct PaneSizes
{
	int treeViewWidth;
	int displayWindowWidth;
	int displayWindowHeight;
};

PaneSizes CalculatePaneSizes(const PaneLayoutSettings &settings);
