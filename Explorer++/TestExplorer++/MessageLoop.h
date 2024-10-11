// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/UniqueThreadId.h"

// Note that the methods in this class are not thread-safe. They should be called from a single
// thread only.
class MessageLoop
{
public:
	MessageLoop();

	// RunUntilIdle() and Run() are designed to be called once.
	void RunUntilIdle();
	void Run();

	void Stop();

private:
	bool PumpMessageLoop();
	void WaitForWork();

	bool m_stop = false;
	bool m_stopWhenIdle = false;
	const UniqueThreadId m_initialThreadId;
};
