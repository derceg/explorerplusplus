// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/Controls.h"
#include <boost/algorithm/string/predicate.hpp>
#include <gtest/gtest.h>
#include <CommCtrl.h>

class DoesComboBoxContainTextTest : public testing::Test
{
protected:
	void SetUp() override
	{
		// Note that there's no message loop here. That should be fine, though, since this window is
		// never visible and only needs to respond to the non-queued messages that will be directly
		// and indirectly sent here. Those messages (sent by SendMessage) will cause the window
		// procedure to be directly invoked, without the need for a message loop.
		m_comboBox = CreateWindow(WC_COMBOBOX, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr);
		ASSERT_NE(m_comboBox, nullptr);

		std::vector<ComboBoxItem> items = { { 0, L"Keyboard" }, { 1, L"Mouse" },
			{ 2, L"Monitor" } };
		AddItemsToComboBox(m_comboBox, items, 0);

		int numItems = ComboBox_GetCount(m_comboBox);
		ASSERT_EQ(static_cast<size_t>(numItems), items.size());
	}

	void TearDown() override
	{
		auto res = DestroyWindow(m_comboBox);
		ASSERT_TRUE(res);
	}

	static bool StringComparator(const std::wstring &input, const std::wstring &test)
	{
		return boost::icontains(input, test);
	}

	HWND m_comboBox = nullptr;
};

TEST_F(DoesComboBoxContainTextTest, Match)
{
	bool containsText = DoesComboBoxContainText(m_comboBox, L"board", StringComparator);
	EXPECT_TRUE(containsText);

	containsText = DoesComboBoxContainText(m_comboBox, L"OUS", StringComparator);
	EXPECT_TRUE(containsText);

	containsText = DoesComboBoxContainText(m_comboBox, L"monit", StringComparator);
	EXPECT_TRUE(containsText);
}

TEST_F(DoesComboBoxContainTextTest, NoMatch)
{
	bool containsText = DoesComboBoxContainText(m_comboBox, L"drive", StringComparator);
	EXPECT_FALSE(containsText);

	containsText = DoesComboBoxContainText(m_comboBox, L"case", StringComparator);
	EXPECT_FALSE(containsText);

	containsText = DoesComboBoxContainText(m_comboBox, L"keys", StringComparator);
	EXPECT_FALSE(containsText);
}
