// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "LocationVisitInfo.h"
#include <string>

class FrequentLocationsModel;

bool operator==(const FrequentLocationsModel &first, const FrequentLocationsModel &second);

namespace FrequentLocationsStorageTestHelper
{

void BuildReferenceModel(FrequentLocationsModel *frequentLocationsModel);
LocationVisitInfo BuildFrequentLocation(const std::wstring &location, int numVisits,
	const SystemClock::TimePoint &lastVisitTime);

}
