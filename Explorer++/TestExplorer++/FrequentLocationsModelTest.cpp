// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsModel.h"
#include "FakeSystemClock.h"
#include "FrequentLocationsStorageTestHelper.h"
#include "ShellTestHelper.h"
#include "../Helper/StringHelper.h"
#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace testing;

namespace
{

void BuildTimePoint(int year, int month, int day, int hour, int minute,
	SystemClock::TimePoint &output)
{
	std::tm tm = { .tm_min = minute,
		.tm_hour = hour,
		.tm_mday = day,
		.tm_mon = month - 1,
		.tm_year = year - 1900 };
	auto time = std::mktime(&tm);
	ASSERT_NE(time, -1);

	output = SystemClock::Clock::from_time_t(time);
}

SystemClock::TimePoint BuildTimePoint(int year, int month, int day, int hour, int minute)
{
	SystemClock::TimePoint timePoint;
	BuildTimePoint(year, month, day, hour, minute, timePoint);
	return timePoint;
}

}

HRESULT PrintPath(const LocationVisitInfo &locationInfo, std::ostream *os)
{
	std::wstring parsingPath;
	HRESULT hr = GetDisplayName(locationInfo.GetLocation().Raw(), SHGDN_FORPARSING, parsingPath);

	if (FAILED(hr))
	{
		return hr;
	}

	auto narrowPath = WstrToStr(parsingPath);

	if (!narrowPath)
	{
		return E_FAIL;
	}

	*os << *narrowPath;

	return S_OK;
}

void PrintTo(const LocationVisitInfo &locationInfo, std::ostream *os)
{
	*os << "LocationVisitInfo(";

	HRESULT hr = PrintPath(locationInfo, os);

	if (FAILED(hr))
	{
		*os << "(Unknown path)";
	}

	*os << ", " << locationInfo.GetNumVisits();
	*os << ", " << locationInfo.GetLastVisitTime();
	*os << ")";
}

class FrequentLocationsModelTest : public Test
{
protected:
	FrequentLocationsModelTest() : m_frequentLocationsModel(&m_systemClock)
	{
	}

	FakeSystemClock m_systemClock;
	FrequentLocationsModel m_frequentLocationsModel;
};

TEST_F(FrequentLocationsModelTest, DifferentLocations)
{
	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	m_frequentLocationsModel.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	m_frequentLocationsModel.RegisterLocationVisit(fake2);

	PidlAbsolute fake3 = CreateSimplePidlForTest(L"C:\\Fake3");
	m_frequentLocationsModel.RegisterLocationVisit(fake3);

	const auto &visits = m_frequentLocationsModel.GetVisits();

	// Items with the same visit count are sorted by their last visit time, so more recent items
	// should appear first.
	std::vector<LocationVisitInfo> expectedVisits = { { fake3, 1, SystemClock::TimePoint(2s) },
		{ fake2, 1, SystemClock::TimePoint(1s) }, { fake1, 1, SystemClock::TimePoint(0s) } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));
}

TEST_F(FrequentLocationsModelTest, RepeatedVisits)
{
	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	m_frequentLocationsModel.RegisterLocationVisit(fake1);
	m_frequentLocationsModel.RegisterLocationVisit(fake1);
	m_frequentLocationsModel.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	m_frequentLocationsModel.RegisterLocationVisit(fake2);

	const auto &visits = m_frequentLocationsModel.GetVisits();

	std::vector<LocationVisitInfo> expectedVisits = { { fake1, 3, SystemClock::TimePoint(2s) },
		{ fake2, 1, SystemClock::TimePoint(3s) } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));
}

TEST_F(FrequentLocationsModelTest, VisitCountOrderChanges)
{
	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	m_frequentLocationsModel.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	m_frequentLocationsModel.RegisterLocationVisit(fake2);
	m_frequentLocationsModel.RegisterLocationVisit(fake2);

	const auto &visits = m_frequentLocationsModel.GetVisits();

	std::vector<LocationVisitInfo> expectedVisits = { { fake2, 2, SystemClock::TimePoint(2s) },
		{ fake1, 1, SystemClock::TimePoint(0s) } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));

	m_frequentLocationsModel.RegisterLocationVisit(fake1);
	m_frequentLocationsModel.RegisterLocationVisit(fake1);

	expectedVisits = { { fake1, 3, SystemClock::TimePoint(4s) },
		{ fake2, 2, SystemClock::TimePoint(2s) } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));

	m_frequentLocationsModel.RegisterLocationVisit(fake2);
	m_frequentLocationsModel.RegisterLocationVisit(fake2);

	expectedVisits = { { fake2, 4, SystemClock::TimePoint(6s) },
		{ fake1, 3, SystemClock::TimePoint(4s) } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));
}

TEST_F(FrequentLocationsModelTest, VisitTimeOrderChanges)
{
	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	m_frequentLocationsModel.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	m_frequentLocationsModel.RegisterLocationVisit(fake2);

	m_frequentLocationsModel.RegisterLocationVisit(fake1);
	m_frequentLocationsModel.RegisterLocationVisit(fake2);

	const auto &visits = m_frequentLocationsModel.GetVisits();

	// fake2 is the most recently visited item, so it should appear first.
	std::vector<LocationVisitInfo> expectedVisits = { { fake2, 2, SystemClock::TimePoint(3s) },
		{ fake1, 2, SystemClock::TimePoint(2s) } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));

	m_frequentLocationsModel.RegisterLocationVisit(fake2);
	m_frequentLocationsModel.RegisterLocationVisit(fake1);

	// fake1 is now the most recently visited item.
	expectedVisits = { { fake1, 3, SystemClock::TimePoint(5s) },
		{ fake2, 3, SystemClock::TimePoint(4s) } };
	EXPECT_THAT(visits, ElementsAreArray(expectedVisits));
}

TEST_F(FrequentLocationsModelTest, LocationsChangedEvent)
{
	MockFunction<void()> callback;
	m_frequentLocationsModel.AddLocationsChangedObserver(callback.AsStdFunction());
	EXPECT_CALL(callback, Call()).Times(4);

	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	m_frequentLocationsModel.RegisterLocationVisit(fake1);
	m_frequentLocationsModel.RegisterLocationVisit(fake1);
	m_frequentLocationsModel.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	m_frequentLocationsModel.RegisterLocationVisit(fake2);
}

TEST_F(FrequentLocationsModelTest, SetLocationVisits)
{
	MockFunction<void()> callback;
	m_frequentLocationsModel.AddLocationsChangedObserver(callback.AsStdFunction());
	EXPECT_CALL(callback, Call());

	auto location1 = FrequentLocationsStorageTestHelper::BuildFrequentLocation(L"c:\\fake1", 4,
		BuildTimePoint(2024, 12, 7, 13, 4));
	auto location2 = FrequentLocationsStorageTestHelper::BuildFrequentLocation(L"c:\\fake2", 28,
		BuildTimePoint(2023, 6, 14, 11, 27));
	auto location3 = FrequentLocationsStorageTestHelper::BuildFrequentLocation(L"c:\\fake3", 19,
		BuildTimePoint(2023, 8, 10, 3, 9));

	m_frequentLocationsModel.SetLocationVisits({ location1, location2, location3 });

	// Once the locations have been added, they should be sorted in descending order of their visit
	// counts.
	EXPECT_THAT(m_frequentLocationsModel.GetVisits(), ElementsAre(location2, location3, location1));
}
