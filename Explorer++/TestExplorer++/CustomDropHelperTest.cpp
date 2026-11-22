// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "CustomDropHelper.h"
#include "ImageTestHelper.h"
#include "ResourceTestHelper.h"
#include "SimulatedClipboardStore.h"
#include "../Helper/Clipboard.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/Pidl.h"
#include <gtest/gtest.h>
#include <Shlwapi.h>

using namespace testing;

class CustomDropHelperTest : public Test
{
protected:
	CustomDropHelperTest() : m_clipboard(&m_clipboardStore)
	{
	}

	void SetUp() override
	{
		// This is a standard filesystem folder that should exist (since its presence is necessary
		// in other tests). So, it should be safe to assume that this folder can accept drop data.
		auto resourcesPath = GetResourcesDirectoryPath();
		HRESULT hr = SHParseDisplayName(resourcesPath.c_str(), nullptr, PidlOutParam(m_directory),
			0, nullptr);
		ASSERT_HRESULT_SUCCEEDED(hr);
	}

	SimulatedClipboardStore m_clipboardStore;
	Clipboard m_clipboard;
	PidlAbsolute m_directory;
};

TEST_F(CustomDropHelperTest, CanPasteInDirectoryWhenEmpty)
{
	// There's no data on the clipboard, so it shouldn't be possible to paste anything.
	EXPECT_FALSE(CustomDropHelper::CanPasteInDirectory(&m_clipboardStore, m_directory.Raw(),
		PasteType::Normal));
	EXPECT_FALSE(CustomDropHelper::CanPasteInDirectory(&m_clipboardStore, m_directory.Raw(),
		PasteType::Shortcut));
}

TEST_F(CustomDropHelperTest, CanPasteTextInDirectory)
{
	ASSERT_TRUE(m_clipboard.WriteText(L"Text content"));
	EXPECT_TRUE(CustomDropHelper::CanPasteInDirectory(&m_clipboardStore, m_directory.Raw(),
		PasteType::Normal));

	// It's not possible to paste a shortcut to text.
	EXPECT_FALSE(CustomDropHelper::CanPasteInDirectory(&m_clipboardStore, m_directory.Raw(),
		PasteType::Shortcut));
}

TEST_F(CustomDropHelperTest, CanPastePngInDirectory)
{
	auto bitmap = BuildTestBitmap(100, 100);
	ASSERT_TRUE(m_clipboard.WritePng(bitmap.get()));
	EXPECT_TRUE(CustomDropHelper::CanPasteInDirectory(&m_clipboardStore, m_directory.Raw(),
		PasteType::Normal));

	// It's not possible to paste a shortcut to an image.
	EXPECT_FALSE(CustomDropHelper::CanPasteInDirectory(&m_clipboardStore, m_directory.Raw(),
		PasteType::Shortcut));
}

TEST_F(CustomDropHelperTest, CanPasteGdiBitmapInDirectory)
{
	auto bitmap = BuildTestBitmap(100, 100);
	ASSERT_TRUE(m_clipboard.WriteGdiBitmap(bitmap.get()));
	EXPECT_TRUE(CustomDropHelper::CanPasteInDirectory(&m_clipboardStore, m_directory.Raw(),
		PasteType::Normal));
	EXPECT_FALSE(CustomDropHelper::CanPasteInDirectory(&m_clipboardStore, m_directory.Raw(),
		PasteType::Shortcut));
}

TEST_F(CustomDropHelperTest, MaybeExtractDataWhenEmpty)
{
	auto fileContents = CustomDropHelper::MaybeExtractData(&m_clipboardStore);
	EXPECT_EQ(fileContents, std::nullopt);
}

TEST_F(CustomDropHelperTest, MaybeExtractTextData)
{
	ASSERT_TRUE(m_clipboard.WriteText(L"Text content"));

	auto fileContents = CustomDropHelper::MaybeExtractData(&m_clipboardStore);
	ASSERT_TRUE(fileContents.has_value());
	EXPECT_EQ(fileContents->type, CustomDropHelper::DataType::Text);
	EXPECT_EQ(fileContents->contents, "Text content");
}

TEST_F(CustomDropHelperTest, MaybeExtractPngData)
{
	auto bitmap = BuildTestBitmap(75, 220);
	ASSERT_TRUE(m_clipboard.WritePng(bitmap.get()));

	auto fileContents = CustomDropHelper::MaybeExtractData(&m_clipboardStore);
	ASSERT_TRUE(fileContents.has_value());
	EXPECT_EQ(fileContents->type, CustomDropHelper::DataType::PngImage);

	wil::com_ptr_nothrow<IStream> stream(
		SHCreateMemStream(reinterpret_cast<const BYTE *>(fileContents->contents.data()),
			static_cast<UINT>(fileContents->contents.size())));
	ASSERT_NE(stream, nullptr);

	wil::com_ptr_nothrow<IWICBitmapSource> outputBitmap;
	ASSERT_HRESULT_SUCCEEDED(ImageHelper::LoadBitmapFromStream(stream.get(), outputBitmap));
	EXPECT_TRUE(AreBitmapsEquivalent(bitmap.get(), outputBitmap.get()));
}

TEST_F(CustomDropHelperTest, MaybeExtractGdiBitmapData)
{
	auto bitmap = BuildTestBitmap(144, 98);
	ASSERT_TRUE(m_clipboard.WriteGdiBitmap(bitmap.get()));

	auto fileContents = CustomDropHelper::MaybeExtractData(&m_clipboardStore);
	ASSERT_TRUE(fileContents.has_value());
	EXPECT_EQ(fileContents->type, CustomDropHelper::DataType::PngImage);

	wil::com_ptr_nothrow<IStream> stream(
		SHCreateMemStream(reinterpret_cast<const BYTE *>(fileContents->contents.data()),
			static_cast<UINT>(fileContents->contents.size())));
	ASSERT_NE(stream, nullptr);

	wil::com_ptr_nothrow<IWICBitmapSource> outputBitmap;
	ASSERT_HRESULT_SUCCEEDED(ImageHelper::LoadBitmapFromStream(stream.get(), outputBitmap));
	EXPECT_TRUE(AreBitmapsEquivalent(bitmap.get(), outputBitmap.get()));
}
