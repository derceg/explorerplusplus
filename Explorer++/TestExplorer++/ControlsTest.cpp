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
		m_comboBox.reset(CreateWindow(WC_COMBOBOX, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_comboBox, nullptr);

		std::vector<ComboBoxItem> items = { { 0, L"Keyboard" }, { 1, L"Mouse" },
			{ 2, L"Monitor" } };
		AddItemsToComboBox(m_comboBox.get(), items, 0);

		int numItems = ComboBox_GetCount(m_comboBox.get());
		ASSERT_EQ(static_cast<size_t>(numItems), items.size());
	}

	void PerformSuccessTest(const std::wstring &text)
	{
		PerformTest(text, true);
	}

	void PerformFailureTest(const std::wstring &text)
	{
		PerformTest(text, false);
	}

private:
	void PerformTest(const std::wstring &text, bool shouldSucceed)
	{
		bool containsText = DoesComboBoxContainText(m_comboBox.get(), text,
			[](const std::wstring &input, const std::wstring &test)
			{ return boost::icontains(input, test); });
		EXPECT_EQ(containsText, shouldSucceed);
	}

	wil::unique_hwnd m_comboBox;
};

TEST_F(DoesComboBoxContainTextTest, Match)
{
	PerformSuccessTest(L"board");
	PerformSuccessTest(L"OUS");
	PerformSuccessTest(L"monit");
}

TEST_F(DoesComboBoxContainTextTest, NoMatch)
{
	PerformFailureTest(L"drive");
	PerformFailureTest(L"case");
	PerformFailureTest(L"keys");
}
