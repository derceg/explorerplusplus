// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

// Allows the RebarView controller to be notified of events that occur within the view.
class RebarViewDelegate
{
public:
	virtual ~RebarViewDelegate() = default;

	virtual void OnShowContextMenu(UINT bandId, const POINT &ptScreen) = 0;
	virtual void OnShowOverflowMenu(UINT bandId, const POINT &ptScreen) = 0;
};
