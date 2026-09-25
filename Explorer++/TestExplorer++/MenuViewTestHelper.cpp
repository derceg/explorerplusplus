// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MenuViewTestHelper.h"
#include "MenuView.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>

namespace MenuViewTestHelper
{

void CheckShellItemDetails(MenuView *menuView, const std::vector<PidlAbsolute> &expectedItems)
{
	ASSERT_EQ(static_cast<size_t>(menuView->GetNumItems()), expectedItems.size());

	for (size_t i = 0; i < expectedItems.size(); i++)
	{
		std::wstring name;
		HRESULT hr = GetDisplayName(expectedItems[i].Raw(), SHGDN_NORMAL, name);
		ASSERT_HRESULT_SUCCEEDED(hr);

		std::wstring path;
		hr = GetDisplayName(expectedItems[i].Raw(), SHGDN_FORPARSING, path);
		ASSERT_HRESULT_SUCCEEDED(hr);

		auto id = menuView->GetItemIdForTesting(static_cast<int>(i));
		EXPECT_EQ(menuView->GetItemTextForTesting(id), name);
		EXPECT_EQ(menuView->GetItemHelpText(id), path);
	}
}

void ExpectItemEnabled(const MenuView *menuView, UINT id, bool enabled)
{
	auto *delegate = menuView->MaybeGetDelegateForItemForTesting(id);
	ASSERT_NE(delegate, nullptr);

	EXPECT_EQ(delegate->IsItemEnabled(id), enabled);
}

void ExpectItemChecked(const MenuView *menuView, UINT id, bool checked)
{
	auto *delegate = menuView->MaybeGetDelegateForItemForTesting(id);
	ASSERT_NE(delegate, nullptr);

	EXPECT_EQ(delegate->IsItemChecked(id), checked);
}

}
