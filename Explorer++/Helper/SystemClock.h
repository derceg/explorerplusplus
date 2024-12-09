// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <chrono>

// This class exists to make it easier to fake the time in unit tests. That is, an implementation
// class can override Now() to provide any desired time within a test.
class SystemClock
{
public:
	using Clock = std::chrono::system_clock;
	using TimePoint = Clock::time_point;

	virtual ~SystemClock() = default;

	virtual TimePoint Now() = 0;
};
