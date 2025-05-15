// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedClipboard.h"
#include <gtest/gtest.h>

using namespace testing;

class SimulatedClipboardTest : public Test
{
protected:
	// Typically, this value would be retrieved using RegisterClipboardFormat(). However, there's
	// not too much point doing that with the simulated clipboard here.
	static constexpr UINT CUSTOM_FORMAT = 1000;

	SimulatedClipboard m_clipboard;
};

TEST_F(SimulatedClipboardTest, InitialData)
{
	auto clipboardText = m_clipboard.ReadText();
	EXPECT_FALSE(clipboardText.has_value());

	auto clipboardCustomData = m_clipboard.ReadCustomData(CUSTOM_FORMAT);
	EXPECT_FALSE(clipboardCustomData.has_value());
}

TEST_F(SimulatedClipboardTest, ReadWriteText)
{
	std::wstring text = L"Test text";
	m_clipboard.WriteText(text);

	auto clipboardText = m_clipboard.ReadText();
	EXPECT_EQ(clipboardText, text);
}

TEST_F(SimulatedClipboardTest, ReadWriteCustomData)
{
	std::string customData = "Custom data";
	m_clipboard.WriteCustomData(CUSTOM_FORMAT, customData);

	auto clipboardCustomData = m_clipboard.ReadCustomData(CUSTOM_FORMAT);
	EXPECT_EQ(clipboardCustomData, customData);
}

TEST_F(SimulatedClipboardTest, MultipleFormats)
{
	std::wstring text = L"Test text";
	m_clipboard.WriteText(text);

	std::string customData = "Custom data";
	m_clipboard.WriteCustomData(CUSTOM_FORMAT, customData);

	auto clipboardText = m_clipboard.ReadText();
	EXPECT_EQ(clipboardText, text);

	auto clipboardCustomData = m_clipboard.ReadCustomData(CUSTOM_FORMAT);
	EXPECT_EQ(clipboardCustomData, customData);
}

TEST_F(SimulatedClipboardTest, Clear)
{
	m_clipboard.WriteText(L"Test text");
	m_clipboard.WriteCustomData(CUSTOM_FORMAT, "Custom data");

	m_clipboard.Clear();

	auto clipboardText = m_clipboard.ReadText();
	EXPECT_FALSE(clipboardText.has_value());

	auto clipboardCustomData = m_clipboard.ReadCustomData(CUSTOM_FORMAT);
	EXPECT_FALSE(clipboardCustomData.has_value());
}
