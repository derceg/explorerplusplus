// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/SystemClock.h"
#include <chrono>

// A fake clock that increments the time by 1 second (starting from the system_clock epoch) every
// time Now() is called.
class FakeSystemClock : public SystemClock
{
public:
	TimePoint Now() override;

private:
	std::chrono::seconds m_secondsSinceEpoch = std::chrono::seconds(0);
};
