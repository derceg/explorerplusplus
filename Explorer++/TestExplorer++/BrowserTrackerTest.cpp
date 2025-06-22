// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserTracker.h"
#include "BrowserList.h"
#include "BrowserWindowMock.h"
#include "GeneratorTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(BrowserTrackerTest, AddRemove)
{
	BrowserList browserList;

	BrowserWindowMock browser1;
	auto browserTracker1 = std::make_unique<BrowserTracker>(&browserList, &browser1);
	EXPECT_THAT(GeneratorToVector(browserList.GetList()), UnorderedElementsAre(&browser1));

	BrowserWindowMock browser2;
	auto browserTracker2 = std::make_unique<BrowserTracker>(&browserList, &browser2);
	EXPECT_THAT(GeneratorToVector(browserList.GetList()),
		UnorderedElementsAre(&browser1, &browser2));

	browserTracker1.reset();
	EXPECT_THAT(GeneratorToVector(browserList.GetList()), UnorderedElementsAre(&browser2));

	browserTracker2.reset();
	EXPECT_THAT(GeneratorToVector(browserList.GetList()), IsEmpty());
}
