// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ComStaThreadPoolExecutor.h"
#include "ExecutorTestBase.h"
#include "../Helper/WindowHelper.h"
#include <gtest/gtest.h>
#include <chrono>
#include <future>

using namespace std::chrono_literals;
using namespace testing;

class ComStaThreadPoolExecutorTest : public ExecutorTestBase
{
protected:
	ComStaThreadPoolExecutorTest() : ExecutorTestBase(std::make_unique<ComStaThreadPoolExecutor>(1))
	{
	}

	void QueueTask(std::function<void()> task)
	{
		auto promise = std::make_shared<std::promise<void>>();

		m_executor->submit(
			[task, promise]
			{
				task();
				promise->set_value();
			});

		auto future = promise->get_future();
		auto res = future.wait_for(TIMEOUT_DURATION);
		ASSERT_EQ(res, std::future_status::ready);
	}

	void QueueMessage(HWND hwnd)
	{
		std::chrono::milliseconds durationMs = TIMEOUT_DURATION;
		auto res = SendMessageTimeout(hwnd, WM_USER_CUSTOM_MESSAGE, 0, 0,
			SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT, static_cast<UINT>(durationMs.count()),
			nullptr);
		ASSERT_NE(res, 0);
	}

private:
	static constexpr UINT WM_USER_CUSTOM_MESSAGE = WM_USER;
	static constexpr std::chrono::duration TIMEOUT_DURATION = 1s;
};

TEST_F(ComStaThreadPoolExecutorTest, QueueMessagesAndTasks)
{
	HWND hwnd = nullptr;

	QueueTask(
		[this, &hwnd]
		{
			// Note that this window isn't explicitly destroyed. That's because the test here is
			// specifically verifying that the executor thread is pumping messages and running
			// tasks. If there's a bug in the executor that either prevents it from pumping
			// messages, or running tasks, attempting to destroy the window isn't necessarily going
			// to work. That is, the window needs to be destroyed by the thread that created it,
			// which is only going to be doable if it's possible to invoke code on that thread.
			// The window will, however, be automatically cleaned up when its creating thread exits
			// (at the end of this test).
			hwnd = CreateMessageOnlyWindow();
		});

	// Assertions from multiple threads aren't currently supported on Windows (see
	// https://google.github.io/googletest/primer.html#known-limitations), which is the reason this
	// value is checked here.
	ASSERT_NE(hwnd, nullptr);

	QueueMessage(hwnd);

	auto task = std::make_shared<MockFunction<void()>>();
	EXPECT_CALL(*task, Call());
	QueueTask([task] { task->Call(); });

	QueueMessage(hwnd);
}

TEST_F(ComStaThreadPoolExecutorTest, CheckComInitialized)
{
	QueueTask(
		[]
		{
			// COM should already have been initialized on this worker thread, with the
			// single-threaded apartment model. The return value here should reflect that.
			HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
			EXPECT_EQ(hr, S_FALSE);
		});
}
