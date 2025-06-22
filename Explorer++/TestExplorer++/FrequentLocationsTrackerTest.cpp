// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsTracker.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "FakeSystemClock.h"
#include "FrequentLocationsModel.h"
#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace testing;

class FrequentLocationsTrackerTest : public BrowserTestBase
{
protected:
	FrequentLocationsTrackerTest() :
		m_frequentLocationsModel(&m_systemClock),
		m_frequentLocationsTracker(&m_frequentLocationsModel, &m_navigationEvents)
	{
	}

	FakeSystemClock m_systemClock;
	FrequentLocationsModel m_frequentLocationsModel;
	FrequentLocationsTracker m_frequentLocationsTracker;
};

TEST_F(FrequentLocationsTrackerTest, CheckNavigationsAdded)
{
	auto *browser1 = AddBrowser();

	PidlAbsolute pidl1;
	browser1->AddTab(L"c:\\path1", {}, &pidl1);

	PidlAbsolute pidl2;
	browser1->AddTab(L"c:\\path2", {}, &pidl2);

	auto *browser2 = AddBrowser();

	PidlAbsolute pidl3;
	browser2->AddTab(L"c:\\path3", {}, &pidl3);

	std::vector<LocationVisitInfo> expectedVisits = { { pidl3, 1, SystemClock::TimePoint(2s) },
		{ pidl2, 1, SystemClock::TimePoint(1s) }, { pidl1, 1, SystemClock::TimePoint(0s) } };
	EXPECT_THAT(m_frequentLocationsModel.GetVisits(), ElementsAreArray(expectedVisits));
}
