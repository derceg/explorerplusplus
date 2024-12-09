// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FakeSystemClock.h"

SystemClock::TimePoint FakeSystemClock::Now()
{
	return TimePoint(m_secondsSinceEpoch++);
}
