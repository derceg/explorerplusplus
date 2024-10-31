// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FeatureList.h"

FeatureList::FeatureList(const std::set<Feature> &featuresToEnable) :
	m_enabledFeatures(featuresToEnable)
{
}

bool FeatureList::IsEnabled(Feature feature) const
{
	return m_enabledFeatures.contains(feature);
}
