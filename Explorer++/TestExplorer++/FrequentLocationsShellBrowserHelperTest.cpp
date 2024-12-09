// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsShellBrowserHelper.h"
#include "FakeSystemClock.h"
#include "FrequentLocationsModel.h"
#include "ShellBrowserHelperTestBase.h"

using namespace std::chrono_literals;
using namespace testing;

class FrequentLocationsShellBrowserHelperTest :
	public ShellBrowserHelperTestBase<FrequentLocationsShellBrowserHelper>
{
protected:
	FrequentLocationsShellBrowserHelperTest() : m_frequentLocationsModel(&m_systemClock)
	{
	}

	void NavigateInNewTab(const std::wstring &path, PidlAbsolute *outputPidl)
	{
		auto shellBrowser = CreateTab(&m_frequentLocationsModel);
		ASSERT_HRESULT_SUCCEEDED(
			shellBrowser->NavigateToPath(path, HistoryEntryType::AddEntry, outputPidl));
	}

	FakeSystemClock m_systemClock;
	FrequentLocationsModel m_frequentLocationsModel;
};

TEST_F(FrequentLocationsShellBrowserHelperTest, NavigationInDifferentTabs)
{
	PidlAbsolute fake1;
	NavigateInNewTab(L"C:\\Fake1", &fake1);

	PidlAbsolute fake2;
	NavigateInNewTab(L"C:\\Fake2", &fake2);

	PidlAbsolute fake3;
	NavigateInNewTab(L"C:\\Fake3", &fake3);

	std::vector<LocationVisitInfo> expectedVisits = { { fake3, 1, SystemClock::TimePoint(2s) },
		{ fake2, 1, SystemClock::TimePoint(1s) }, { fake1, 1, SystemClock::TimePoint(0s) } };
	EXPECT_THAT(m_frequentLocationsModel.GetVisits(), ElementsAreArray(expectedVisits));
}
