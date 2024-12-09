// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LocationVisitInfo.h"

LocationVisitInfo::LocationVisitInfo(const PidlAbsolute &pidl, int numVisits,
	const SystemClock::TimePoint &lastVisitTime) :
	m_pidl(pidl),
	m_numVisits(std::max(numVisits, 1)),
	m_lastVisitTime(lastVisitTime)
{
}

void LocationVisitInfo::AddVisit(const SystemClock::TimePoint &currentTime)
{
	m_numVisits++;
	m_lastVisitTime = currentTime;
}

PidlAbsolute LocationVisitInfo::GetLocation() const
{
	return m_pidl;
}

int LocationVisitInfo::GetNumVisits() const
{
	return m_numVisits;
}

SystemClock::TimePoint LocationVisitInfo::GetLastVisitTime() const
{
	return m_lastVisitTime;
}
