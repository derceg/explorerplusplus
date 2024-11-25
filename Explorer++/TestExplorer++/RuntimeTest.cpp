// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Runtime.h"
#include "ComStaThreadPoolExecutor.h"
#include "ExecutorTestHelper.h"
#include "UIThreadExecutor.h"
#include <gtest/gtest.h>

TEST(RuntimeTest, RetrievalMethods)
{
	auto uiThreadExecutor = std::make_shared<UIThreadExecutor>();
	auto rawUiThreadExecutor = uiThreadExecutor.get();
	auto comStaExecutor = std::make_shared<ComStaThreadPoolExecutor>(1);
	auto rawComStaExecutor = comStaExecutor.get();

	Runtime runtime(std::move(uiThreadExecutor), std::move(comStaExecutor));

	EXPECT_EQ(runtime.GetUiThreadExecutor().get(), rawUiThreadExecutor);
	EXPECT_EQ(runtime.GetComStaExecutor().get(), rawComStaExecutor);
	EXPECT_TRUE(runtime.IsUiThread());

	RunTaskOnExecutorForTest(runtime.GetComStaExecutor(),
		[&runtime] { EXPECT_FALSE(runtime.IsUiThread()); });
}
