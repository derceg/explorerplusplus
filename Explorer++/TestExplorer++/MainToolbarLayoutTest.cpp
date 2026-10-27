// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MainToolbarLayout.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(MainToolbarLayoutTest, DuplicateButtons)
{
	MainToolbarLayout layout;
	layout.AddButton(MainToolbarButtonId::Up);
	layout.AddButton(MainToolbarButtonId::OpenCommandPrompt);
	EXPECT_THAT(layout.GetButtons(),
		ElementsAre(MainToolbarButtonId::Up, MainToolbarButtonId::OpenCommandPrompt));

	// It shouldn't be possible to add a standard button more than once, so this call should have no
	// effect.
	layout.AddButton(MainToolbarButtonId::Up);
	EXPECT_THAT(layout.GetButtons(),
		ElementsAre(MainToolbarButtonId::Up, MainToolbarButtonId::OpenCommandPrompt));
}

TEST(MainToolbarLayoutTest, DuplicateSeparators)
{
	MainToolbarLayout layout;
	layout.AddButton(MainToolbarButtonId::Properties);
	layout.AddButton(MainToolbarButtonId::Separator);
	layout.AddButton(MainToolbarButtonId::Refresh);
	EXPECT_THAT(layout.GetButtons(),
		ElementsAre(MainToolbarButtonId::Properties, MainToolbarButtonId::Separator,
			MainToolbarButtonId::Refresh));

	// Unlike standard buttons, it should be possible for separators to appear multiple times.
	layout.AddButton(MainToolbarButtonId::Separator);
	EXPECT_THAT(layout.GetButtons(),
		ElementsAre(MainToolbarButtonId::Properties, MainToolbarButtonId::Separator,
			MainToolbarButtonId::Refresh, MainToolbarButtonId::Separator));
}
