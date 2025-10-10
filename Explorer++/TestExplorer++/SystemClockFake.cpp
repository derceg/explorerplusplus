// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SystemClockFake.h"

SystemClock::TimePoint SystemClockFake::Now()
{
	return TimePoint(m_secondsSinceEpoch++);
}
