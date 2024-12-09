// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SystemClockImpl.h"

SystemClock::TimePoint SystemClockImpl::Now()
{
	return Clock::now();
}
