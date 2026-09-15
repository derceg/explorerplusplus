// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MassRenameTokensMenu.h"
#include "AcceleratorManager.h"
#include "MassRenameHelper.h"
#include "MenuTestHost.h"
#include "ResourceLoaderFake.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(MassRenameTokensMenuTest, Selection)
{
	AcceleratorManager acceleratorManager;
	ResourceLoaderFake resourceLoader;

	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	MockFunction<void(MassRenameToken token)> selectionCallback;
	MassRenameTokensMenu menu(menuView, &acceleratorManager, selectionCallback.AsStdFunction(),
		&resourceLoader);

	InSequence seq;

	for (auto token : MassRenameToken::_values())
	{
		EXPECT_CALL(selectionCallback, Call(token));
	}

	for (int i = 0; i < menuView->GetNumItems(); i++)
	{
		menuHost.SelectItemAtIndex(i, false, false);
	}
}
