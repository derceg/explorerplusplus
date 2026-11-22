// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/DataExchangeHelper.h"
#include "ImageTestHelper.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/GraphicsFactories.h"
#include <gtest/gtest.h>
#include <Shlwapi.h>

using namespace testing;

TEST(DataExchangeHelperTest, ReadPngDataFromGlobalProducesNewBitmap)
{
	auto bitmap = BuildTestBitmap(10, 10);
	auto global = WritePngDataToGlobal(bitmap.get());
	auto retrievedBitmap = ReadPngDataFromGlobal(global.get());

	global.reset();

	auto *imagingFactory = GraphicsFactories::GetWICFactory();

	// The bitmap produced by ReadPngDataFromGlobal() should stay valid, even if the backing global
	// has been destroyed. Which means this call should succeed without triggering any errors.
	wil::com_ptr_nothrow<IWICBitmap> retrievedBitmapCopy;
	ASSERT_HRESULT_SUCCEEDED(imagingFactory->CreateBitmapFromSource(retrievedBitmap.get(),
		WICBitmapCacheOnLoad, &retrievedBitmapCopy));
}

TEST(DataExchangeHelperTest, ReadWriteVirtualFiles)
{
	auto dataObject = winrt::make<DataObjectImpl>();

	std::vector<VirtualFile> virtualFiles = { { L"test-file-1.txt", "Contents of file 1" },
		{ L"test-file-2.txt", "Contents of file 2" },
		{ L"test-file-3.txt", "Contents of file 3" } };
	ASSERT_HRESULT_SUCCEEDED(WriteVirtualFilesToDataObject(dataObject.get(), virtualFiles));

	std::vector<VirtualFile> retrievedVirtualFiles;
	ASSERT_HRESULT_SUCCEEDED(
		ReadVirtualFilesFromDataObject(dataObject.get(), retrievedVirtualFiles));

	EXPECT_EQ(retrievedVirtualFiles, virtualFiles);
}

TEST(DataExchangeHelperTest, CloneGlobal)
{
	std::wstring text = L"Test text";
	auto global = WriteStringToGlobal(text);
	ASSERT_NE(global, nullptr);

	auto clone = CloneGlobal(global.get());
	ASSERT_NE(clone, nullptr);

	auto retrievedText = ReadStringFromGlobal(clone.get());
	EXPECT_EQ(retrievedText, text);
}

TEST(DataExchangeHelperTest, GetBinaryDataFromIStream)
{
	std::string data = "Test data";
	wil::com_ptr_nothrow<IStream> stream(SHCreateMemStream(
		reinterpret_cast<const BYTE *>(data.data()), static_cast<UINT>(data.size())));
	ASSERT_NE(stream, nullptr);

	std::string output;
	ASSERT_HRESULT_SUCCEEDED(GetBinaryDataFromIStream(stream.get(), output));
	EXPECT_EQ(output, data);
}
