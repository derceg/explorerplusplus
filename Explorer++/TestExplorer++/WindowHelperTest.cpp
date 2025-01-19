// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/WindowHelper.h"
#include <gtest/gtest.h>
#include <CommCtrl.h>

using namespace testing;

class WindowHelperTest : public Test
{
protected:
	void SetUp() override
	{
		m_window.reset(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_window, nullptr);
	}

	wil::unique_hwnd m_window;
};

TEST_F(WindowHelperTest, GetWindowString)
{
	const std::wstring text = L"Test window text";
	auto res = SetWindowText(m_window.get(), text.c_str());
	ASSERT_TRUE(res);

	EXPECT_EQ(GetWindowString(m_window.get()), text);
}

TEST_F(WindowHelperTest, AddWindowStyles)
{
	LONG_PTR originalStyle = GetWindowLongPtr(m_window.get(), GWL_STYLE);
	ASSERT_NE(originalStyle, 0);

	AddWindowStyles(m_window.get(), WS_BORDER | WS_TABSTOP, true);
	EXPECT_EQ(GetWindowLongPtr(m_window.get(), GWL_STYLE), originalStyle | WS_BORDER | WS_TABSTOP);

	AddWindowStyles(m_window.get(), WS_TABSTOP, false);
	EXPECT_EQ(GetWindowLongPtr(m_window.get(), GWL_STYLE), originalStyle | WS_BORDER);
}
