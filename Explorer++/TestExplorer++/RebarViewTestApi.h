// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class RebarView;

class RebarViewTestApi
{
public:
	RebarViewTestApi(const RebarView *view);

	bool IsBandShown(UINT id) const;
	bool IsBandOnNewLine(UINT id) const;
	bool DoesBandUseChevron(UINT id) const;
	bool IsBandGripperEnabled(UINT id) const;
	int GetBandIndex(UINT id) const;
	UINT GetBandWidth(UINT id) const;
	UINT GetBandIdealWidth(UINT id) const;
	UINT GetBandMinimumHeight(UINT id) const;

	bool AreBandsLocked() const;
	int GetNumBands() const;

private:
	const RebarView *const m_view;
};
