// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ExecutorTestHelper.h"
#include <future>

void RunTaskOnExecutorForTest(std::shared_ptr<concurrencpp::executor> executor,
	std::function<void()> task)
{
	auto promise = std::make_shared<std::promise<void>>();

	executor->submit(
		[task, promise]
		{
			task();
			promise->set_value();
		});

	auto future = promise->get_future();
	auto res = future.wait_for(TASK_TIMEOUT_DURATION);
	ASSERT_EQ(res, std::future_status::ready);
}
