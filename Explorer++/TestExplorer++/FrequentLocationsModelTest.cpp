// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsModel.h"
#include "FrequentLocationsStorageTestHelper.h"
#include "ShellTestHelper.h"
#include "../Helper/StringHelper.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{

void BuildTimePoint(int year, int month, int day, int hour, int minute,
	std::chrono::system_clock::time_point &output)
{
	std::tm tm = { .tm_min = minute,
		.tm_hour = hour,
		.tm_mday = day,
		.tm_mon = month - 1,
		.tm_year = year - 1900 };
	auto time = std::mktime(&tm);
	ASSERT_NE(time, -1);

	output = std::chrono::system_clock::from_time_t(time);
}

std::chrono::system_clock::time_point BuildTimePoint(int year, int month, int day, int hour,
	int minute)
{
	std::chrono::system_clock::time_point timePoint;
	BuildTimePoint(year, month, day, hour, minute, timePoint);
	return timePoint;
}

}

MATCHER(MatchLocationAndVisits, "")
{
	// The last visit time is ignored here.
	return std::get<0>(arg).GetLocation() == std::get<1>(arg).GetLocation()
		&& std::get<0>(arg).GetNumVisits() == std::get<1>(arg).GetNumVisits();
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
	*os << ")";
}

TEST(FrequentLocationsModelTest, DifferentLocations)
{
	FrequentLocationsModel frequentLocationsModel;

	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	frequentLocationsModel.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	frequentLocationsModel.RegisterLocationVisit(fake2);

	PidlAbsolute fake3 = CreateSimplePidlForTest(L"C:\\Fake3");
	frequentLocationsModel.RegisterLocationVisit(fake3);

	const auto &visits = frequentLocationsModel.GetVisits();

	// Items with the same visit count are sorted by their last visit time, so more recent items
	// should appear first.
	std::vector<LocationVisitInfo> expectedVisits = { { fake3, 1 }, { fake2, 1 }, { fake1, 1 } };
	EXPECT_THAT(visits, Pointwise(MatchLocationAndVisits(), expectedVisits));
}

TEST(FrequentLocationsModelTest, RepeatedVisits)
{
	FrequentLocationsModel frequentLocationsModel;

	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	frequentLocationsModel.RegisterLocationVisit(fake1);
	frequentLocationsModel.RegisterLocationVisit(fake1);
	frequentLocationsModel.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	frequentLocationsModel.RegisterLocationVisit(fake2);

	const auto &visits = frequentLocationsModel.GetVisits();

	std::vector<LocationVisitInfo> expectedVisits = { { fake1, 3 }, { fake2, 1 } };
	EXPECT_THAT(visits, Pointwise(MatchLocationAndVisits(), expectedVisits));
}

TEST(FrequentLocationsModelTest, VisitCountOrderChanges)
{
	FrequentLocationsModel frequentLocationsModel;

	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	frequentLocationsModel.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	frequentLocationsModel.RegisterLocationVisit(fake2);
	frequentLocationsModel.RegisterLocationVisit(fake2);

	const auto &visits = frequentLocationsModel.GetVisits();

	std::vector<LocationVisitInfo> expectedVisits = { { fake2, 2 }, { fake1, 1 } };
	EXPECT_THAT(visits, Pointwise(MatchLocationAndVisits(), expectedVisits));

	frequentLocationsModel.RegisterLocationVisit(fake1);
	frequentLocationsModel.RegisterLocationVisit(fake1);

	expectedVisits = { { fake1, 3 }, { fake2, 2 } };
	EXPECT_THAT(visits, Pointwise(MatchLocationAndVisits(), expectedVisits));

	frequentLocationsModel.RegisterLocationVisit(fake2);
	frequentLocationsModel.RegisterLocationVisit(fake2);

	expectedVisits = { { fake2, 4 }, { fake1, 3 } };
	EXPECT_THAT(visits, Pointwise(MatchLocationAndVisits(), expectedVisits));
}

TEST(FrequentLocationsModelTest, VisitTimeOrderChanges)
{
	FrequentLocationsModel frequentLocationsModel;

	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	frequentLocationsModel.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	frequentLocationsModel.RegisterLocationVisit(fake2);

	frequentLocationsModel.RegisterLocationVisit(fake1);
	frequentLocationsModel.RegisterLocationVisit(fake2);

	const auto &visits = frequentLocationsModel.GetVisits();

	// fake2 is the most recently visited item, so it should appear first.
	std::vector<LocationVisitInfo> expectedVisits = { { fake2, 2 }, { fake1, 2 } };
	EXPECT_THAT(visits, Pointwise(MatchLocationAndVisits(), expectedVisits));

	frequentLocationsModel.RegisterLocationVisit(fake2);
	frequentLocationsModel.RegisterLocationVisit(fake1);

	// fake1 is now the most recently visited item.
	expectedVisits = { { fake1, 3 }, { fake2, 3 } };
	EXPECT_THAT(visits, Pointwise(MatchLocationAndVisits(), expectedVisits));
}

TEST(FrequentLocationsModelTest, LocationsChangedEvent)
{
	FrequentLocationsModel frequentLocationsModel;

	MockFunction<void()> callback;
	frequentLocationsModel.AddLocationsChangedObserver(callback.AsStdFunction());
	EXPECT_CALL(callback, Call()).Times(4);

	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	frequentLocationsModel.RegisterLocationVisit(fake1);
	frequentLocationsModel.RegisterLocationVisit(fake1);
	frequentLocationsModel.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	frequentLocationsModel.RegisterLocationVisit(fake2);
}

TEST(FrequentLocationsModelTest, SetLocationVisits)
{
	FrequentLocationsModel frequentLocationsModel;

	MockFunction<void()> callback;
	frequentLocationsModel.AddLocationsChangedObserver(callback.AsStdFunction());
	EXPECT_CALL(callback, Call());

	using namespace std::chrono_literals;

	auto location1 = FrequentLocationsStorageTestHelper::BuildFrequentLocation(L"c:\\fake1", 4,
		BuildTimePoint(2024, 12, 7, 13, 4));
	auto location2 = FrequentLocationsStorageTestHelper::BuildFrequentLocation(L"c:\\fake2", 28,
		BuildTimePoint(2023, 6, 14, 11, 27));
	auto location3 = FrequentLocationsStorageTestHelper::BuildFrequentLocation(L"c:\\fake3", 19,
		BuildTimePoint(2023, 8, 10, 3, 9));

	frequentLocationsModel.SetLocationVisits({ location1, location2, location3 });

	// Once the locations have been added, they should be sorted in descending order of their visit
	// counts.
	EXPECT_THAT(frequentLocationsModel.GetVisits(), ElementsAre(location2, location3, location1));
}
