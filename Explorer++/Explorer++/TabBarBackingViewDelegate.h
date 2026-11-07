// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class TabBarBackingViewDelegate
{
public:
	virtual ~TabBarBackingViewDelegate() = default;

	virtual void OnDoubleClicked() = 0;
	virtual void OnShowContextMenu(const POINT &ptScreen) = 0;
};
