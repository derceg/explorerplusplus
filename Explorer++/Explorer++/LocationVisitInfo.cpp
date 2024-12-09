// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LocationVisitInfo.h"

LocationVisitInfo::LocationVisitInfo(const PidlAbsolute &pidl, int numVisits,
	const Clock::time_point &lastVisitTime) :
	m_pidl(pidl),
	m_numVisits(std::max(numVisits, 1)),
	m_lastVisitTime(lastVisitTime)
{
}

void LocationVisitInfo::AddVisit()
{
	m_numVisits++;
	m_lastVisitTime = Clock::now();
}

PidlAbsolute LocationVisitInfo::GetLocation() const
{
	return m_pidl;
}

int LocationVisitInfo::GetNumVisits() const
{
	return m_numVisits;
}

LocationVisitInfo::Clock::time_point LocationVisitInfo::GetLastVisitTime() const
{
	return m_lastVisitTime;
}
