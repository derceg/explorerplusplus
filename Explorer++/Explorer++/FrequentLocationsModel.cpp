// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FrequentLocationsModel.h"

FrequentLocationsModel::FrequentLocationsModel(SystemClock *systemClock) :
	m_systemClock(systemClock)
{
}

void FrequentLocationsModel::SetLocationVisits(const std::vector<LocationVisitInfo> &locationVisits)
{
	m_locationVisits.clear();
	m_locationVisits.insert(locationVisits.begin(), locationVisits.end());
	m_locationsChangedSignal();
}

void FrequentLocationsModel::RegisterLocationVisit(const PidlAbsolute &pidl)
{
	auto &locationIndex = m_locationVisits.get<ByLocation>();
	auto itr = locationIndex.find(pidl);

	if (itr == locationIndex.end())
	{
		locationIndex.emplace(pidl, 1, m_systemClock->Now());
	}
	else
	{
		locationIndex.modify(itr,
			[this](auto &locationInfo) { locationInfo.AddVisit(m_systemClock->Now()); });
	}

	m_locationsChangedSignal();
}

const FrequentLocationsModel::ByVisitsIndex &FrequentLocationsModel::GetVisits() const
{
	return m_locationVisits.get<ByVisits>();
}

boost::signals2::connection FrequentLocationsModel::AddLocationsChangedObserver(
	const LocationsChangedSignal::slot_type &observer)
{
	return m_locationsChangedSignal.connect(observer);
}
