// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RuntimeHelper.h"
#include "ExecutorTestHelper.h"
#include "MessageLoop.h"
#include "Runtime.h"
#include "RuntimeTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class RuntimeHelperTest : public Test
{
protected:
	RuntimeHelperTest() : m_runtime(BuildRuntimeForTest())
	{
	}

	concurrencpp::null_result RunTaskOnUiThread(const Runtime *runtime, std::function<void()> task)
	{
		co_await ResumeOnUiThread(runtime);
		task();
	}

	Runtime m_runtime;
};

TEST_F(RuntimeHelperTest, ResumeOnUiThreadFromUiThread)
{
	MockFunction<void()> task1;
	MockFunction<void()> task2;

	// As ResumeOnUiThread() is being called from the UI thread, task1 should be run inline.
	InSequence sequence;
	EXPECT_CALL(task1, Call());
	EXPECT_CALL(task2, Call());

	RunTaskOnUiThread(&m_runtime, task1.AsStdFunction());
	task2.Call();
}

TEST_F(RuntimeHelperTest, ResumeOnUiThreadFromBackgroundThread)
{
	auto task1 = std::make_shared<MockFunction<void()>>();
	auto task2 = std::make_shared<MockFunction<void()>>();

	// As ResumeOnUiThread() is being called from a background thread, task1 should be queued and
	// run only once the message loop has been processed.
	InSequence sequence;
	EXPECT_CALL(*task2, Call());
	EXPECT_CALL(*task1, Call());

	RunTaskOnExecutorForTest(m_runtime.GetComStaExecutor(),
		[this, task1, task2]
		{
			RunTaskOnUiThread(&m_runtime, task1->AsStdFunction());
			task2->Call();
		});

	// At this point, the lambda above has been invoked on the COM STA executor. The only thing left
	// to do is pump messages until the RunTaskOnUiThread() coroutine completes.
	MessageLoop messageLoop;
	messageLoop.RunUntilIdle();
}
