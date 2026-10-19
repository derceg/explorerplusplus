// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class RebarBandView;

// For a view that can be inserted into a rebar, this represents the controller side.
class RebarBand
{
public:
	virtual ~RebarBand() = default;

	virtual RebarBandView *GetView() = 0;

	virtual bool CanShowOverflowMenu() const
	{
		return false;
	}

	virtual void ShowOverflowMenu(const POINT &ptScreen)
	{
		UNREFERENCED_PARAMETER(ptScreen);
	}
};
