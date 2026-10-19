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
	UINT GetHeight() const override;
	std::optional<UINT> MaybeGetIdealWidth() const override;

	void SetHeight(UINT height);
	void SetIdealWidth(UINT idealWidth);

private:
	const HWND m_hwnd;
	UINT m_height = 100;
	std::optional<UINT> m_idealWidth;
};
