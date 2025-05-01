// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ScopedBrowserCommandTarget.h"
#include "BrowserCommandTargetFake.h"
#include "BrowserCommandTargetManager.h"
#include <gtest/gtest.h>

TEST(ScopedBrowserCommandTargetTest, SetClear)
{
	BrowserCommandTargetManager targetManager;

	BrowserCommandTargetFake target;
	auto scopedTarget = std::make_unique<ScopedBrowserCommandTarget>(&targetManager, &target);
	EXPECT_NE(targetManager.GetCurrentTarget(), &target);

	scopedTarget->TargetFocused();
	EXPECT_EQ(targetManager.GetCurrentTarget(), &target);

	scopedTarget.reset();
	EXPECT_NE(targetManager.GetCurrentTarget(), &target);
}
