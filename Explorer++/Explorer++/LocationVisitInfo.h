// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/PidlHelper.h"
#include <chrono>

// Holds information about visits to a particular location (identified by its pidl).
class LocationVisitInfo
{
public:
	using Clock = std::chrono::system_clock;

	LocationVisitInfo(const PidlAbsolute &pidl, int numVisits = 1,
		const Clock::time_point &lastVisitTime = Clock::now());

	void AddVisit();
	PidlAbsolute GetLocation() const;
	int GetNumVisits() const;
	Clock::time_point GetLastVisitTime() const;

	// This is only used in tests.
	bool operator==(const LocationVisitInfo &) const = default;

private:
	PidlAbsolute m_pidl;
	int m_numVisits;
	Clock::time_point m_lastVisitTime;
};
