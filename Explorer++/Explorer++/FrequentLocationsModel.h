// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "LocationVisitInfo.h"
#include "../Helper/PidlHelper.h"
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/signals2.hpp>

// Stores information about how often locations are visited. A sorted set of frequently visited
// locations can be retrieved.
class FrequentLocationsModel
{
public:
	using LocationsChangedSignal = boost::signals2::signal<void()>;

	// This struct and the one below are used as tags with the multi_index_container. That is, they
	// allow specific indices to be accessed in a more descriptive way (rather than by the index
	// order number).
	struct ByVisits
	{
	};

	struct ByLocation
	{
	};

	// clang-format off
	using LocationVisits = boost::multi_index_container<LocationVisitInfo,
		boost::multi_index::indexed_by<
			// An index of visits, sorted by visit count and then last visit time (when the visit
			// counts are the same).
			boost::multi_index::ordered_non_unique<
				boost::multi_index::tag<ByVisits>,
				boost::multi_index::composite_key<
					LocationVisitInfo,
					boost::multi_index::const_mem_fun<LocationVisitInfo, int, &LocationVisitInfo::GetNumVisits>,
					boost::multi_index::const_mem_fun<LocationVisitInfo, LocationVisitInfo::Clock::time_point,
						&LocationVisitInfo::GetLastVisitTime>
				>,
				// Items are sorted in descending order of visit count (i.e. most visited first) and
				// descending order of last visit time (i.e. most recently visited items first).
				boost::multi_index::composite_key_compare<
					std::greater<int>,
					std::greater<LocationVisitInfo::Clock::time_point>
				>
			>,
			// A non-sorted index of visits, based on the pidl.
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<ByLocation>,
				boost::multi_index::const_mem_fun<LocationVisitInfo, PidlAbsolute, &LocationVisitInfo::GetLocation>
			>
		>
	>;
	// clang-format on

	using ByVisitsIndex = LocationVisits::index<ByVisits>::type;

	void SetLocationVisits(const std::vector<LocationVisitInfo> &locationVisits);
	void RegisterLocationVisit(const PidlAbsolute &pidl);
	const ByVisitsIndex &GetVisits() const;
	boost::signals2::connection AddLocationsChangedObserver(
		const LocationsChangedSignal::slot_type &observer);

private:
	LocationVisits m_locationVisits;
	LocationsChangedSignal m_locationsChangedSignal;
};
