// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "UniqueThreadId.h"
#include <atomic>

UniqueThreadId UniqueThreadId::GetForCurrentThread()
{
	static std::atomic_int threadIdCounter = 0;
	static thread_local int currentThreadId = -1;

	if (currentThreadId == -1)
	{
		currentThreadId = threadIdCounter++;
	}

	return UniqueThreadId(currentThreadId);
}

UniqueThreadId::UniqueThreadId(int threadId) : m_threadId(threadId)
{
}
