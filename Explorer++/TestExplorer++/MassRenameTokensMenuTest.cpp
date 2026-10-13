// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MassRenameTokensMenu.h"
#include "AcceleratorManager.h"
#include "MassRenameHelper.h"
#include "ResourceLoaderFake.h"
#include "RootMenuViewTestApi.h"
#include "TestMenuView.h"
#include <gtest/gtest.h>
#include <magic_enum/magic_enum.hpp>

using namespace testing;

TEST(MassRenameTokensMenuTest, Selection)
{
	AcceleratorManager acceleratorManager;
	ResourceLoaderFake resourceLoader;

	TestMenuView menuView;
	MockFunction<void(MassRenameToken token)> selectionCallback;
	MassRenameTokensMenu menu(&menuView, &acceleratorManager, selectionCallback.AsStdFunction(),
		&resourceLoader);

	InSequence seq;

	for (auto token : magic_enum::enum_values<MassRenameToken>())
	{
		EXPECT_CALL(selectionCallback, Call(token));
	}

	for (int i = 0; i < menuView.GetNumItems(); i++)
	{
		RootMenuViewTestApi::SelectItemAtIndex(&menuView, i, false, false);
	}
}
