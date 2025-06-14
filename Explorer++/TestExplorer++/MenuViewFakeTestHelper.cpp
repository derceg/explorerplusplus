// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MenuViewFakeTestHelper.h"
#include "MenuViewFake.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>

namespace MenuViewFakeTestHelper
{

void CheckItemDetails(MenuViewFake *menuView, const std::vector<PidlAbsolute> &expectedItems)
{
	ASSERT_EQ(static_cast<size_t>(menuView->GetItemCount()), expectedItems.size());

	for (size_t i = 0; i < expectedItems.size(); i++)
	{
		std::wstring name;
		HRESULT hr = GetDisplayName(expectedItems[i].Raw(), SHGDN_NORMAL, name);
		ASSERT_HRESULT_SUCCEEDED(hr);

		std::wstring path;
		hr = GetDisplayName(expectedItems[i].Raw(), SHGDN_FORPARSING, path);
		ASSERT_HRESULT_SUCCEEDED(hr);

		auto id = menuView->GetItemId(static_cast<int>(i));
		EXPECT_EQ(menuView->GetItemText(id), name);
		EXPECT_EQ(menuView->GetItemHelpText(id), path);
	}
}

}
