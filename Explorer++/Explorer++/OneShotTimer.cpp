// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "OneShotTimer.h"

OneShotTimer::OneShotTimer(OneShotTimerManager *timerManager) : m_timerManager(timerManager)
{
}

OneShotTimer::~OneShotTimer()
{
	Stop();
}

void OneShotTimer::Start(std::chrono::milliseconds delay, OneShotTimerManager::Callback callback)
{
	Stop();

	// This call may technically fail, but it's not expected to happen and there's not much that
	// can be done if it does fail. Therefore, any failure here is ignored.
	m_timerId = m_timerManager->StartTimer(delay, callback);
}

void OneShotTimer::Stop()
{
	if (!m_timerId)
	{
		return;
	}

	m_timerManager->StopTimer(*m_timerId);
	m_timerId.reset();
}

std::optional<UINT_PTR> OneShotTimer::GetTimerIdForTesting() const
{
	return m_timerId;
}
