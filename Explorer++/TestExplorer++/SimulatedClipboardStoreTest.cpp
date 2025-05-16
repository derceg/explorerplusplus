// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedClipboardStore.h"
#include "../Helper/DataExchangeHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class SimulatedClipboardStoreTest : public Test
{
protected:
	// Typically, this value would be retrieved using RegisterClipboardFormat(). However, there's
	// not too much point doing that with the simulated clipboard here.
	static constexpr UINT CUSTOM_FORMAT = 1000;

	std::optional<std::wstring> ReadText() const
	{
		auto clipboardData = m_clipboardStore.GetData(CF_UNICODETEXT);

		if (!clipboardData)
		{
			return std::nullopt;
		}

		return ReadStringFromGlobal(clipboardData);
	}

	void WriteText(const std::wstring &text)
	{
		auto global = WriteStringToGlobal(text);
		ASSERT_NE(global, nullptr);

		m_clipboardStore.SetData(CF_UNICODETEXT, std::move(global));
	}

	std::optional<std::string> ReadCustomData() const
	{
		auto clipboardData = m_clipboardStore.GetData(CUSTOM_FORMAT);

		if (!clipboardData)
		{
			return std::nullopt;
		}

		return ReadBinaryDataFromGlobal(clipboardData);
	}

	void WriteCustomData(const std::string &data)
	{
		auto global = WriteDataToGlobal(data.data(), data.size() * sizeof(char));
		ASSERT_NE(global, nullptr);

		m_clipboardStore.SetData(CUSTOM_FORMAT, std::move(global));
	}

	SimulatedClipboardStore m_clipboardStore;
};

TEST_F(SimulatedClipboardStoreTest, IsDataAvailable)
{
	EXPECT_FALSE(m_clipboardStore.IsDataAvailable(CF_UNICODETEXT));

	WriteText(L"Test text");
	EXPECT_TRUE(m_clipboardStore.IsDataAvailable(CF_UNICODETEXT));

	EXPECT_FALSE(m_clipboardStore.IsDataAvailable(CUSTOM_FORMAT));

	WriteCustomData("Custom data");
	EXPECT_TRUE(m_clipboardStore.IsDataAvailable(CUSTOM_FORMAT));
}

TEST_F(SimulatedClipboardStoreTest, GetSetData)
{
	std::wstring text = L"Test text";
	WriteText(text);

	auto clipboardText = ReadText();
	EXPECT_EQ(clipboardText, text);
}

TEST_F(SimulatedClipboardStoreTest, GetSetDataMultipleFormats)
{
	std::wstring text = L"Test text";
	WriteText(text);

	std::string customData = "Custom data";
	WriteCustomData(customData);

	auto clipboardText = ReadText();
	EXPECT_EQ(clipboardText, text);

	auto clipboardCustomData = ReadCustomData();
	EXPECT_EQ(clipboardCustomData, customData);
}

TEST_F(SimulatedClipboardStoreTest, Clear)
{
	WriteText(L"Test text");

	m_clipboardStore.Clear();

	EXPECT_FALSE(m_clipboardStore.IsDataAvailable(CF_UNICODETEXT));
	EXPECT_EQ(m_clipboardStore.GetData(CF_UNICODETEXT), nullptr);
}
