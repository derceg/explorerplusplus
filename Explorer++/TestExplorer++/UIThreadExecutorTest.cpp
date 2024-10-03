// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "UIThreadExecutor.h"
#include "ExecutorTestBase.h"
#include <gtest/gtest.h>

using namespace testing;

class UIThreadExecutorTest : public ExecutorTestBase
{
protected:
	UIThreadExecutorTest() : ExecutorTestBase(std::make_unique<UIThreadExecutor>())
	{
	}

	void PumpMessageLoopUntilIdle()
	{
		MSG msg;

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
};

TEST_F(UIThreadExecutorTest, Submit)
{
	MockFunction<void()> task1;
	m_executor->submit(task1.AsStdFunction());
	EXPECT_CALL(task1, Call());

	MockFunction<void()> task2;
	m_executor->submit(task2.AsStdFunction());
	EXPECT_CALL(task2, Call());

	PumpMessageLoopUntilIdle();
}

TEST_F(UIThreadExecutorTest, BulkSubmit)
{
	std::vector<MockFunction<void()>> tasks(4);
	std::vector<std::function<void()>> tasksAsFunctions;

	for (auto &task : tasks)
	{
		EXPECT_CALL(task, Call());

		tasksAsFunctions.push_back(task.AsStdFunction());
	}

	m_executor->bulk_submit<std::function<void()>>(tasksAsFunctions);

	PumpMessageLoopUntilIdle();
}

TEST_F(UIThreadExecutorTest, ShutdownRequested)
{
	EXPECT_FALSE(m_executor->shutdown_requested());

	m_executor->shutdown();
	EXPECT_TRUE(m_executor->shutdown_requested());
}

TEST_F(UIThreadExecutorTest, ShutdownDuringTaskLoop)
{
	// If shutdown() is called while a task is being run, any remaining tasks should be skipped.
	MockFunction<void()> task1;
	m_executor->submit(task1.AsStdFunction());
	EXPECT_CALL(task1, Call()).WillOnce([this] { m_executor->shutdown(); });

	MockFunction<void()> task2;
	m_executor->submit(task2.AsStdFunction());
	EXPECT_CALL(task2, Call()).Times(0);

	PumpMessageLoopUntilIdle();
}

TEST_F(UIThreadExecutorTest, EnqueueAfterShutdown)
{
	m_executor->shutdown();

	EXPECT_THROW(m_executor->enqueue(concurrencpp::task()), concurrencpp::errors::runtime_shutdown);

	concurrencpp::task tasks[4];
	std::span<concurrencpp::task> tasksSpan = tasks;
	EXPECT_THROW(m_executor->enqueue(tasksSpan), concurrencpp::errors::runtime_shutdown);
}
