// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "OneShotTimer.h"
#include "OneShotTimerManager.h"
#include "../Helper/MessageWindowHelper.h"
#include "../Helper/WindowHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <CommCtrl.h>

using namespace testing;
using namespace std::chrono_literals;

class OneShotTimerTest : public Test
{
protected:
	OneShotTimerTest() :
		m_messageWindow(MessageWindowHelper::CreateMessageOnlyWindow()),
		m_timerManager(std::make_unique<OneShotTimerManager>(m_messageWindow.get())),
		m_timer(std::make_unique<OneShotTimer>(m_timerManager.get()))
	{
	}

	wil::unique_hwnd m_messageWindow;
	std::unique_ptr<OneShotTimerManager> m_timerManager;
	std::unique_ptr<OneShotTimer> m_timer;
	MockFunction<void()> m_callback;
};

TEST_F(OneShotTimerTest, TriggerCallback)
{
	m_timer->Start(0s, m_callback.AsStdFunction());

	auto timerId = m_timer->GetTimerIdForTesting();
	ASSERT_NE(timerId, std::nullopt);

	EXPECT_CALL(m_callback, Call());
	SendMessage(m_messageWindow.get(), WM_TIMER, *timerId, 0);
}

TEST_F(OneShotTimerTest, MultipleTimerMessages)
{
	m_timer->Start(0s, m_callback.AsStdFunction());

	auto timerId = m_timer->GetTimerIdForTesting();
	ASSERT_NE(timerId, std::nullopt);

	SendMessage(m_messageWindow.get(), WM_TIMER, *timerId, 0);

	// The timer has already been triggered once, so it shouldn't be triggered if another WM_TIMER
	// message is received.
	EXPECT_CALL(m_callback, Call()).Times(0);
	SendMessage(m_messageWindow.get(), WM_TIMER, *timerId, 0);
}

TEST_F(OneShotTimerTest, NestedMessage)
{
	std::optional<UINT_PTR> timerId;

	// This simulates a nested message loop. In practice, what could happen is that the callback
	// causes a dialog to be shown, a nested message loop is run, and (if the timer hasn't been
	// killed) a WM_TIMER message is dispatched and processed while the dialog is being shown,
	// before the callback has finished running.
	// This verifies that the callback will only be run once, even if a nested message is processed.
	auto callback = [this, &timerId]()
	{
		m_callback.Call();

		SendMessage(m_messageWindow.get(), WM_TIMER, *timerId, 0);
	};

	m_timer->Start(0s, callback);

	timerId = m_timer->GetTimerIdForTesting();
	ASSERT_NE(timerId, std::nullopt);

	EXPECT_CALL(m_callback, Call()).Times(1);
	SendMessage(m_messageWindow.get(), WM_TIMER, *timerId, 0);
}

TEST_F(OneShotTimerTest, RestartTimer)
{
	m_timer->Start(0s, m_callback.AsStdFunction());

	auto originalTimerId = m_timer->GetTimerIdForTesting();
	ASSERT_NE(originalTimerId, std::nullopt);

	m_timer->Start(0s, m_callback.AsStdFunction());

	auto updatedTimerId = m_timer->GetTimerIdForTesting();
	ASSERT_NE(updatedTimerId, std::nullopt);
	EXPECT_NE(*updatedTimerId, *originalTimerId);

	// A WM_TIMER for the original timer should be ignored, since the second call to Start() should
	// have invalidated it.
	EXPECT_CALL(m_callback, Call()).Times(0);
	SendMessage(m_messageWindow.get(), WM_TIMER, *originalTimerId, 0);

	// A WM_TIMER message for the updated timer should still be processed.
	EXPECT_CALL(m_callback, Call());
	SendMessage(m_messageWindow.get(), WM_TIMER, *updatedTimerId, 0);
}

TEST_F(OneShotTimerTest, StopTimer)
{
	m_timer->Start(0s, m_callback.AsStdFunction());

	auto timerId = m_timer->GetTimerIdForTesting();
	ASSERT_NE(timerId, std::nullopt);

	m_timer->Stop();

	// A timer should never be triggered once it has been stopped.
	EXPECT_CALL(m_callback, Call()).Times(0);
	SendMessage(m_messageWindow.get(), WM_TIMER, *timerId, 0);
}

TEST_F(OneShotTimerTest, DestroyTimer)
{
	m_timer->Start(0s, m_callback.AsStdFunction());

	auto timerId = m_timer->GetTimerIdForTesting();
	ASSERT_NE(timerId, std::nullopt);

	m_timer.reset();

	// Destroying the timer should prevent it from being triggered.
	EXPECT_CALL(m_callback, Call()).Times(0);
	SendMessage(m_messageWindow.get(), WM_TIMER, *timerId, 0);
}
