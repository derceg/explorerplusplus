// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FrequentLocationsService.h"

void FrequentLocationsService::RegisterLocationVisit(const PidlAbsolute &pidl)
{
	auto &locationIndex = m_locationVisits.get<ByLocation>();
	auto itr = locationIndex.find(pidl);

	if (itr == locationIndex.end())
	{
		locationIndex.emplace(pidl);
	}
	else
	{
		locationIndex.modify(itr, [](auto &locationInfo) { locationInfo.AddVisit(); });
	}

	m_locationsChangedSignal();
}

const FrequentLocationsService::ByVisitsIndex &FrequentLocationsService::GetVisits() const
{
	return m_locationVisits.get<ByVisits>();
}

boost::signals2::connection FrequentLocationsService::AddLocationsChangedObserver(
	const LocationsChangedSignal::slot_type &observer)
{
	return m_locationsChangedSignal.connect(observer);
}
