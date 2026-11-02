// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RebarBandView.h"

std::optional<int> RebarBandView::MaybeGetIdealWidth() const
{
	return std::nullopt;
}

boost::signals2::connection RebarBandView::AddSizeRequirementsChangedObserver(
	const SizeRequirementsChangedSignal::slot_type &observer)
{
	return m_sizeRequirementsChangedSignal.connect(observer);
}

void RebarBandView::NotifySizeRequirementsChanged()
{
	m_sizeRequirementsChangedSignal();
}
