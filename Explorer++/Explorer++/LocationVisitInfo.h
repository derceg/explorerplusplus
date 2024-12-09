// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include "../Helper/SystemClock.h"

// Holds information about visits to a particular location (identified by its pidl).
class LocationVisitInfo
{
public:
	LocationVisitInfo(const PidlAbsolute &pidl, int numVisits,
		const SystemClock::TimePoint &lastVisitTime);

	void AddVisit(const SystemClock::TimePoint &currentTime);
	PidlAbsolute GetLocation() const;
	int GetNumVisits() const;
	SystemClock::TimePoint GetLastVisitTime() const;

	// This is only used in tests.
	bool operator==(const LocationVisitInfo &) const = default;

private:
	PidlAbsolute m_pidl;
	int m_numVisits;
	SystemClock::TimePoint m_lastVisitTime;
};
