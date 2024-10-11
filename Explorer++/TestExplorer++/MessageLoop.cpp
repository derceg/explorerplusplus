// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MessageLoop.h"

MessageLoop::MessageLoop() : m_initialThreadId(UniqueThreadId::GetForCurrentThread())
{
}

void MessageLoop::RunUntilIdle()
{
	DCHECK(UniqueThreadId::GetForCurrentThread() == m_initialThreadId);

	m_stopWhenIdle = true;
	Run();
}

void MessageLoop::Run()
{
	DCHECK(UniqueThreadId::GetForCurrentThread() == m_initialThreadId);

	while (true)
	{
		while (PumpMessageLoop())
			;

		// Because this class is designed to be called from a single thread, it's only possible to
		// call Stop() from a processed message (either directly, or indirectly). Which means that
		// after PumpMessageLoop() has been called, m_stop may have transitioned from false to true.
		// It's not possible for m_stop to be set within WaitForWork(), as this thread will block
		// during that call.
		if (m_stop || m_stopWhenIdle)
		{
			break;
		}

		WaitForWork();
	}
}

void MessageLoop::Stop()
{
	DCHECK(UniqueThreadId::GetForCurrentThread() == m_initialThreadId);

	m_stop = true;
}

bool MessageLoop::PumpMessageLoop()
{
	MSG msg;
	BOOL res = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);

	if (!res)
	{
		return false;
	}

	TranslateMessage(&msg);
	DispatchMessage(&msg);

	return true;
}

void MessageLoop::WaitForWork()
{
	MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT, 0);
}
