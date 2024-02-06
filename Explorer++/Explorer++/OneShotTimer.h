// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "OneShotTimerManager.h"
#include <chrono>
#include <optional>

// Represents a one-shot timer. That is, a timer that's guaranteed to only be called once after
// being started, or not at all if the timer is stopped before being triggered.
class OneShotTimer
{
public:
	OneShotTimer(OneShotTimerManager *timerManager);
	~OneShotTimer();

	void Start(std::chrono::milliseconds delay, OneShotTimerManager::Callback callback);
	void Stop();

	std::optional<UINT_PTR> GetTimerIdForTesting() const;

private:
	OneShotTimerManager *m_timerManager = nullptr;
	std::optional<UINT_PTR> m_timerId;
};
