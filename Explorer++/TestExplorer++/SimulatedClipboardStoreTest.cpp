// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedClipboardStore.h"
#include "GTestHelper.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>
#include <wil/com.h>

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

		return ReadStringFromGlobal(clipboardData.get());
	}

	void WriteText(const std::wstring &text)
	{
		auto global = WriteStringToGlobal(text);
		ASSERT_NE(global, nullptr);

		ASSERT_TRUE(m_clipboardStore.SetData(CF_UNICODETEXT, std::move(global)));
	}

	std::optional<std::string> ReadCustomData() const
	{
		auto clipboardData = m_clipboardStore.GetData(CUSTOM_FORMAT);

		if (!clipboardData)
		{
			return std::nullopt;
		}

		return ReadBinaryDataFromGlobal(clipboardData.get());
	}

	void WriteCustomData(const std::string &data)
	{
		auto global = WriteDataToGlobal(data.data(), data.size() * sizeof(char));
		ASSERT_NE(global, nullptr);

		ASSERT_TRUE(m_clipboardStore.SetData(CUSTOM_FORMAT, std::move(global)));
	}

	SimulatedClipboardStore m_clipboardStore;
};

TEST_F(SimulatedClipboardStoreTest, InitialState)
{
	EXPECT_FALSE(m_clipboardStore.IsDataAvailable(CF_UNICODETEXT));
	EXPECT_FALSE(m_clipboardStore.IsDataAvailable(CUSTOM_FORMAT));

	auto clipboardText = ReadText();
	EXPECT_FALSE(clipboardText.has_value());

	auto clipboardCustomData = ReadCustomData();
	EXPECT_FALSE(clipboardCustomData.has_value());

	// There should be a data object available, even if no data has been written.
	auto dataObject = m_clipboardStore.GetDataObject();
	ASSERT_NE(dataObject, nullptr);

	wil::com_ptr_nothrow<IEnumFORMATETC> enumFormats;
	ASSERT_HRESULT_SUCCEEDED(dataObject->EnumFormatEtc(DATADIR_GET, &enumFormats));

	int numFormats = 0;
	unique_formatetc format;

	while (enumFormats->Next(1, &format, nullptr) == S_OK)
	{
		numFormats++;
	}

	// The data object shouldn't have any formats available.
	EXPECT_EQ(numFormats, 0);
}

TEST_F(SimulatedClipboardStoreTest, IsDataAvailable)
{
	WriteText(L"Test text");
	EXPECT_TRUE(m_clipboardStore.IsDataAvailable(CF_UNICODETEXT));

	EXPECT_FALSE(m_clipboardStore.IsDataAvailable(CUSTOM_FORMAT));

	WriteCustomData("Custom data");
	EXPECT_TRUE(m_clipboardStore.IsDataAvailable(CUSTOM_FORMAT));
}

TEST_F(SimulatedClipboardStoreTest, IsDataAvailableDataObject)
{
	auto dataObject = winrt::make<DataObjectImpl>();

	std::wstring text = L"Test text";
	ASSERT_HRESULT_SUCCEEDED(SetTextOnDataObject(dataObject.get(), text));

	ASSERT_TRUE(m_clipboardStore.SetDataObject(dataObject.get()));
	EXPECT_TRUE(m_clipboardStore.IsDataAvailable(CF_UNICODETEXT));
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

TEST_F(SimulatedClipboardStoreTest, GetSetDataObject)
{
	auto dataObject = winrt::make<DataObjectImpl>();

	std::wstring text = L"Test text";
	ASSERT_HRESULT_SUCCEEDED(SetTextOnDataObject(dataObject.get(), text));

	std::string customData = "Custom data";
	ASSERT_HRESULT_SUCCEEDED(SetBlobData(dataObject.get(), CUSTOM_FORMAT, customData.data(),
		customData.size() * sizeof(char)));

	ASSERT_TRUE(m_clipboardStore.SetDataObject(dataObject.get()));

	auto clipboardDataObject = m_clipboardStore.GetDataObject();
	ASSERT_NE(clipboardDataObject, nullptr);

	std::wstring clipboardText;
	ASSERT_HRESULT_SUCCEEDED(GetTextFromDataObject(clipboardDataObject.get(), clipboardText));
	EXPECT_EQ(clipboardText, text);

	std::string clipboardCustomData;
	ASSERT_HRESULT_SUCCEEDED(
		GetBlobData(clipboardDataObject.get(), CUSTOM_FORMAT, clipboardCustomData));
	EXPECT_EQ(clipboardCustomData, customData);
}

TEST_F(SimulatedClipboardStoreTest, SetDataObjectClearsClipboard)
{
	std::wstring text = L"Test text";
	WriteText(text);

	auto dataObject = winrt::make<DataObjectImpl>();
	ASSERT_TRUE(m_clipboardStore.SetDataObject(dataObject.get()));

	auto clipboardText = ReadText();
	EXPECT_FALSE(clipboardText.has_value());
}

TEST_F(SimulatedClipboardStoreTest, GetDataAfterSetDataObject)
{
	auto dataObject = winrt::make<DataObjectImpl>();

	std::wstring text = L"Test text";
	ASSERT_HRESULT_SUCCEEDED(SetTextOnDataObject(dataObject.get(), text));

	ASSERT_TRUE(m_clipboardStore.SetDataObject(dataObject.get()));

	auto clipboardText = ReadText();
	EXPECT_EQ(clipboardText, text);
}

TEST_F(SimulatedClipboardStoreTest, GetDataObjectAfterSetData)
{
	std::wstring text = L"Test text";
	WriteText(text);

	auto clipboardDataObject = m_clipboardStore.GetDataObject();
	ASSERT_NE(clipboardDataObject, nullptr);

	std::wstring clipboardText;
	ASSERT_HRESULT_SUCCEEDED(GetTextFromDataObject(clipboardDataObject.get(), clipboardText));
	EXPECT_EQ(clipboardText, text);
}

TEST_F(SimulatedClipboardStoreTest, IsDataObjectCurrent)
{
	auto dataObject = winrt::make<DataObjectImpl>();
	ASSERT_TRUE(m_clipboardStore.SetDataObject(dataObject.get()));
	EXPECT_TRUE(m_clipboardStore.IsDataObjectCurrent(dataObject.get()));

	auto dataObject2 = winrt::make<DataObjectImpl>();
	EXPECT_FALSE(m_clipboardStore.IsDataObjectCurrent(dataObject2.get()));
}

TEST_F(SimulatedClipboardStoreTest, Clear)
{
	WriteText(L"Test text");

	ASSERT_TRUE(m_clipboardStore.Clear());

	EXPECT_FALSE(m_clipboardStore.IsDataAvailable(CF_UNICODETEXT));
	EXPECT_EQ(m_clipboardStore.GetData(CF_UNICODETEXT), nullptr);
}

using SimulatedClipboardStoreDeathTest = SimulatedClipboardStoreTest;

TEST_F(SimulatedClipboardStoreDeathTest, SetDataOnDataObject)
{
	auto clipboardDataObject = m_clipboardStore.GetDataObject();
	ASSERT_NE(clipboardDataObject, nullptr);

	auto global = WriteStringToGlobal(L"Test text");
	ASSERT_NE(global, nullptr);

	FORMATETC format = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	auto stg = GetStgMediumForGlobal(std::move(global));

	// Any attempt to set data on the IDataObject instance returned by GetDataObject() should fail.
	EXPECT_CHECK_DEATH(clipboardDataObject->SetData(&format, &stg, false));
}
