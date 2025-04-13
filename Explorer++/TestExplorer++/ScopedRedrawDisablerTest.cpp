// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ScopedRedrawDisabler.h"
#include "../Helper/MessageWindowHelper.h"
#include <gtest/gtest.h>
#include <VersionHelpers.h>

using namespace testing;

class ScopedRedrawDisablerTest : public Test
{
protected:
	ScopedRedrawDisablerTest() : m_window(MessageWindowHelper::CreateMessageOnlyWindow())
	{
	}

	bool IsRedrawEnabled()
	{
		// The return value is 0 when redraw is enabled.
		return GetProp(m_window.get(), REDRAW_PROP_NAME) == 0;
	}

	const wil::unique_hwnd m_window;

private:
	static constexpr wchar_t REDRAW_PROP_NAME[] = L"SysSetRedraw";
};

TEST_F(ScopedRedrawDisablerTest, CheckRedrawState)
{
	// The method used to detect whether redraw is enabled/disabled is only valid on Windows 10 and
	// above. If this test is being run on an earlier version of Windows, it should be skipped.
	if (!IsWindows10OrGreater())
	{
		GTEST_SKIP();
	}

	// The window needs to be visible for the default redraw handling to do anything. Note that
	// since this is a message-only window, it won't actually be shown.
	ShowWindow(m_window.get(), SW_SHOW);

	EXPECT_TRUE(IsRedrawEnabled());

	auto redrawDisabler = std::make_unique<ScopedRedrawDisabler>(m_window.get());

	EXPECT_FALSE(IsRedrawEnabled());

	redrawDisabler.reset();

	EXPECT_TRUE(IsRedrawEnabled());
}
