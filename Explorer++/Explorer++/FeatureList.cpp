// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FeatureList.h"

namespace FeatureList
{
bool IsEnabled(Feature feature)
{
	switch (feature)
	{
	case Feature::DualPane:
		return false;

	default:
		throw std::runtime_error("Invalid feature");
	}
}
}
