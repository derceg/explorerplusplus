// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RebarViewTestApi.h"
#include "RebarView.h"
#include <wil/common.h>
#include <commctrl.h>

RebarViewTestApi::RebarViewTestApi(const RebarView *view) : m_view(view)
{
}

bool RebarViewTestApi::IsBandShown(UINT id) const
{
	auto styles = m_view->GetBandStyles(id);
	return WI_IsFlagClear(styles, RBBS_HIDDEN);
}

bool RebarViewTestApi::IsBandOnNewLine(UINT id) const
{
	auto styles = m_view->GetBandStyles(id);
	return WI_IsFlagSet(styles, RBBS_BREAK);
}

bool RebarViewTestApi::DoesBandUseChevron(UINT id) const
{
	auto styles = m_view->GetBandStyles(id);
	return WI_IsFlagSet(styles, RBBS_USECHEVRON);
}

bool RebarViewTestApi::IsBandGripperEnabled(UINT id) const
{
	auto styles = m_view->GetBandStyles(id);
	return WI_IsFlagClear(styles, RBBS_NOGRIPPER);
}

int RebarViewTestApi::GetBandIndex(UINT id) const
{
	return m_view->GetBandIndex(id);
}

UINT RebarViewTestApi::GetBandWidth(UINT id) const
{
	auto bandInfo = m_view->GetBandInfo(id, RBBIM_SIZE);
	return bandInfo.cx;
}

UINT RebarViewTestApi::GetBandIdealWidth(UINT id) const
{
	auto bandInfo = m_view->GetBandInfo(id, RBBIM_IDEALSIZE);
	return bandInfo.cxIdeal;
}

UINT RebarViewTestApi::GetBandMinimumHeight(UINT id) const
{
	auto bandInfo = m_view->GetBandInfo(id, RBBIM_CHILDSIZE);
	return bandInfo.cyMinChild;
}

bool RebarViewTestApi::AreBandsLocked() const
{
	return m_view->m_locked;
}

int RebarViewTestApi::GetNumBands() const
{
	return m_view->GetNumBands();
}
