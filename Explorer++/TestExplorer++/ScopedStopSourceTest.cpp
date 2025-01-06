// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ScopedStopSource.h"
#include <gtest/gtest.h>

TEST(ScopedStopSourceTest, CheckStopRequested)
{
	auto scopedStopSource = std::make_unique<ScopedStopSource>();
	auto token = scopedStopSource->GetToken();
	EXPECT_FALSE(token.stop_requested());

	scopedStopSource.reset();

	EXPECT_TRUE(token.stop_requested());
}
