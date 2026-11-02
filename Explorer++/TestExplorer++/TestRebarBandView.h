// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RebarBandView.h"

class TestRebarBandView : public RebarBandView
{
public:
	TestRebarBandView(HWND parent);

	// RebarBandView
	HWND GetHWND() const override;
	int GetHeight() const override;
	std::optional<int> MaybeGetIdealWidth() const override;

	void SetHeight(int height);
	void SetIdealWidth(int idealWidth);

private:
	const HWND m_hwnd;
	int m_height = 100;
	std::optional<int> m_idealWidth;
};
