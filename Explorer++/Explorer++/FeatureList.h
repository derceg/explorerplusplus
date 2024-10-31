// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Feature.h"
#include <set>

// This class can be used to determine whether a particular in-development feature is enabled or
// disabled. All the features managed here are disabled by default, but can be enabled with the
// appropriate command line option.
class FeatureList
{
public:
	FeatureList(const std::set<Feature> &featuresToEnable);
	bool IsEnabled(Feature feature) const;

private:
	std::set<Feature> m_enabledFeatures;
};
