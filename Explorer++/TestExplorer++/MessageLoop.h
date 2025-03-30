// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/UniqueThreadId.h"
#include <concurrencpp/concurrencpp.h>
#include <memory>

class UIThreadExecutor;

// Note that the methods in this class are not thread-safe. They should be called from a single
// thread only.
class MessageLoop
{
public:
	MessageLoop();

	// These methods are designed to be called once.
	void RunWithTimeout(std::chrono::milliseconds timeout,
		std::shared_ptr<UIThreadExecutor> uiThreadExecutor);
	void RunUntilIdle();
	void Run();

	void Stop();

private:
	bool PumpMessageLoop();
	void WaitForWork();

	bool m_stop = false;
	bool m_stopWhenIdle = false;
	const UniqueThreadId m_initialThreadId;
	const std::shared_ptr<concurrencpp::timer_queue> m_timerQueue;
	concurrencpp::timer m_timeoutTimer;
};
