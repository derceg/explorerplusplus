// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FeatureList.h"
#include <gtest/gtest.h>

TEST(FeatureListTest, DisabledByDefault)
{
	FeatureList featureList({});
	EXPECT_FALSE(featureList.IsEnabled(Feature::Plugins));
	EXPECT_FALSE(featureList.IsEnabled(Feature::DualPane));
}

TEST(FeatureListTest, Enabled)
{
	FeatureList featureList({ Feature::Plugins });
	EXPECT_TRUE(featureList.IsEnabled(Feature::Plugins));
	EXPECT_FALSE(featureList.IsEnabled(Feature::DualPane));
}
