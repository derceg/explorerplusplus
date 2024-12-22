// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "PopupMenuViewTestHelper.h"
#include "PopupMenuView.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>

namespace PopupMenuViewTestHelper
{

void CheckItemDetails(PopupMenuView *popupMenu, const std::vector<PidlAbsolute> &expectedItems)
{
	ASSERT_EQ(static_cast<size_t>(popupMenu->GetItemCountForTesting()), expectedItems.size());

	for (size_t i = 0; i < expectedItems.size(); i++)
	{
		std::wstring name;
		HRESULT hr = GetDisplayName(expectedItems[i].Raw(), SHGDN_NORMAL, name);
		ASSERT_HRESULT_SUCCEEDED(hr);

		std::wstring path;
		hr = GetDisplayName(expectedItems[i].Raw(), SHGDN_FORPARSING, path);
		ASSERT_HRESULT_SUCCEEDED(hr);

		auto id = popupMenu->GetItemIdForTesting(static_cast<int>(i));
		EXPECT_EQ(popupMenu->GetItemTextForTesting(id), name);
		EXPECT_EQ(popupMenu->GetHelpTextForItem(id), path);
	}
}

}
