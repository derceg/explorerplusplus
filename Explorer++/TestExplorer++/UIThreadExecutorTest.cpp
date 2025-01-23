// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "UIThreadExecutor.h"
#include "ExecutorTestBase.h"
#include "MessageLoop.h"
#include <gtest/gtest.h>

using namespace testing;

class UIThreadExecutorTest : public ExecutorTestBase
{
protected:
	UIThreadExecutorTest() : ExecutorTestBase(std::make_shared<UIThreadExecutor>())
	{
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

	MessageLoop messageLoop;
	messageLoop.RunUntilIdle();
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

	MessageLoop messageLoop;
	messageLoop.RunUntilIdle();
}

TEST_F(UIThreadExecutorTest, NestedTasks)
{
	MockFunction<void(int)> check;
	{
		InSequence seq;

		// The first task should fully complete before the second task runs, so the calls it makes
		// should take place before the call the second task makes.
		EXPECT_CALL(check, Call(1));
		EXPECT_CALL(check, Call(2));
		EXPECT_CALL(check, Call(3));
	}

	m_executor->submit(
		[this, &check]()
		{
			check.Call(1);

			m_executor->submit([&check] { check.Call(3); });

			// Run a nested message loop by pumping messages. This shouldn't result in the second
			// task being immediately run; instead, the second task should start only once this task
			// has finished.
			MessageLoop messageLoop;
			messageLoop.RunUntilIdle();

			check.Call(2);
		});

	MessageLoop messageLoop;
	messageLoop.RunUntilIdle();
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

	MessageLoop messageLoop;
	messageLoop.RunUntilIdle();
}

TEST_F(UIThreadExecutorTest, EnqueueAfterShutdown)
{
	m_executor->shutdown();

	EXPECT_THROW(m_executor->enqueue(concurrencpp::task()), concurrencpp::errors::runtime_shutdown);

	concurrencpp::task tasks[4];
	std::span<concurrencpp::task> tasksSpan = tasks;
	EXPECT_THROW(m_executor->enqueue(tasksSpan), concurrencpp::errors::runtime_shutdown);
}
