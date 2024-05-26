// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DataExchangeHelper.h"
#include "GdiplusHelper.h"
#include "ScopedBitmapLock.h"
#include <wil/com.h>

std::optional<std::wstring> ReadStringFromGlobal(HGLOBAL global)
{
	wil::unique_hglobal_locked mem(global);

	if (!mem)
	{
		return std::nullopt;
	}

	auto size = GlobalSize(mem.get());

	if (size == 0)
	{
		return std::nullopt;
	}

	return std::wstring(static_cast<const WCHAR *>(mem.get()), (size / sizeof(WCHAR)) - 1);
}

wil::unique_hglobal WriteStringToGlobal(const std::wstring &str)
{
	return WriteDataToGlobal(str.c_str(), (str.size() + 1) * sizeof(WCHAR));
}

std::optional<std::string> ReadBinaryDataFromGlobal(HGLOBAL global)
{
	wil::unique_hglobal_locked mem(global);

	if (!mem)
	{
		return std::nullopt;
	}

	auto size = GlobalSize(mem.get());

	if (size == 0)
	{
		return std::nullopt;
	}

	return std::string(static_cast<const char *>(mem.get()), size);
}

wil::unique_hglobal WriteBinaryDataToGlobal(const std::string &data)
{
	return WriteDataToGlobal(data.data(), data.size());
}

wil::unique_hglobal WriteDataToGlobal(const void *data, size_t size)
{
	wil::unique_hglobal global(GlobalAlloc(GMEM_MOVEABLE, size));

	if (!global)
	{
		return nullptr;
	}

	wil::unique_hglobal_locked mem(global.get());

	if (!mem)
	{
		return nullptr;
	}

	memcpy(mem.get(), data, size);

	return global;
}

std::optional<std::vector<std::wstring>> ReadHDropDataFromGlobal(HGLOBAL global)
{
	wil::unique_hglobal_locked mem(global);

	if (!mem)
	{
		return std::nullopt;
	}

	auto dropData = static_cast<HDROP>(mem.get());
	std::vector<std::wstring> paths;

	UINT numFiles = DragQueryFile(dropData, 0xFFFFFFFF, nullptr, 0);

	for (UINT i = 0; i < numFiles; i++)
	{
		UINT numCharacters = DragQueryFile(dropData, i, nullptr, 0);

		if (numCharacters == 0)
		{
			continue;
		}

		// The return value of DragQueryFile() doesn't include space for the terminating null
		// character.
		std::wstring path;
		path.resize(numCharacters + 1);

		numCharacters = DragQueryFile(dropData, i, path.data(), static_cast<UINT>(path.size()));

		if (numCharacters == 0)
		{
			continue;
		}

		path.resize(numCharacters);

		paths.push_back(path);
	}

	if (paths.empty())
	{
		return std::nullopt;
	}

	return paths;
}

wil::unique_hglobal WriteHDropDataToGlobal(const std::vector<std::wstring> &paths)
{
	if (paths.empty())
	{
		// An empty list of filenames isn't valid.
		return nullptr;
	}

	std::wstring concatenatedPaths;

	for (const auto &path : paths)
	{
		concatenatedPaths.append(path);
		concatenatedPaths.append(1, '\0');
	}

	// The list of filenames needs to be double null-terminated.
	concatenatedPaths.append(1, '\0');

	size_t headerSize = sizeof(DROPFILES);
	size_t concatenatedPathsSize = concatenatedPaths.size() * sizeof(WCHAR);

	wil::unique_hglobal global(GlobalAlloc(GHND, headerSize + concatenatedPathsSize));

	if (!global)
	{
		return nullptr;
	}

	wil::unique_hglobal_locked mem(global.get());

	if (!mem)
	{
		return nullptr;
	}

	auto dropData = static_cast<DROPFILES *>(mem.get());
	dropData->pFiles = static_cast<DWORD>(headerSize);
	dropData->fWide = true;

	auto *filenameData = reinterpret_cast<std::byte *>(dropData) + headerSize;
	std::memcpy(filenameData, concatenatedPaths.data(), concatenatedPathsSize);

	return global;
}

std::unique_ptr<Gdiplus::Bitmap> ReadPngDataFromGlobal(HGLOBAL global)
{
	wil::com_ptr_nothrow<IStream> stream;
	HRESULT hr = CreateStreamOnHGlobal(global, false, &stream);

	if (FAILED(hr))
	{
		return nullptr;
	}

	auto bitmap = std::make_unique<Gdiplus::Bitmap>(stream.get());

	if (bitmap->GetLastStatus() != Gdiplus::Ok)
	{
		return nullptr;
	}

	// The Gdiplus::Bitmap constructor above doesn't copy the data passed to it. That means that the
	// input HGLOBAL would have to remain valid for as long as the bitmap exists. That doesn't align
	// with how this function is designed to work (the caller shouldn't have worry about the
	// lifetime of the input parameters after this function has returned), which is the reason why a
	// deep copy of the bitmap is returned here.
	return GdiplusHelper::DeepCopyBitmap(bitmap.get());
}

wil::unique_hglobal WritePngDataToGlobal(Gdiplus::Bitmap *bitmap)
{
	auto pngClsid = GdiplusHelper::GetEncoderClsid(L"image/png");

	if (!pngClsid)
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<IStream> stream;
	HRESULT hr = CreateStreamOnHGlobal(nullptr, false, &stream);

	if (FAILED(hr))
	{
		return nullptr;
	}

	// As noted in https://devblogs.microsoft.com/oldnewthing/20210929-00/?p=105742, if this
	// function were to fail, the global inside the stream would be leaked. However, that post also
	// notes that this function is guaranteed to succeed if the stream it's given came from
	// CreateStreamOnHGlobal(), hence the CHECK().
	wil::unique_hglobal global;
	hr = GetHGlobalFromStream(stream.get(), &global);
	CHECK(SUCCEEDED(hr));

	auto status = bitmap->Save(stream.get(), &pngClsid.value(), nullptr);

	if (status != Gdiplus::Ok)
	{
		return nullptr;
	}

	return global;
}

std::unique_ptr<Gdiplus::Bitmap> ReadDIBDataFromGlobal(HGLOBAL global)
{
	wil::unique_hglobal_locked mem(global);

	if (!mem)
	{
		return nullptr;
	}

	auto *bitmapInfo = static_cast<BITMAPINFO *>(mem.get());

	int colorTableLength = 0;

	// See
	// https://source.chromium.org/chromium/chromium/src/+/main:ui/base/clipboard/clipboard_win.cc;l=833;drc=177693cc196465bf71d8d0c0fab8c4f1bb9d95b4.
	switch (bitmapInfo->bmiHeader.biBitCount)
	{
	case 1:
	case 4:
	case 8:
		colorTableLength = bitmapInfo->bmiHeader.biClrUsed ? bitmapInfo->bmiHeader.biClrUsed
														   : 1 << bitmapInfo->bmiHeader.biBitCount;
		break;

	case 16:
	case 32:
		if (bitmapInfo->bmiHeader.biCompression == BI_BITFIELDS)
		{
			colorTableLength = 3;
		}
		break;
	}

	void *data = reinterpret_cast<std::byte *>(bitmapInfo) + bitmapInfo->bmiHeader.biSize
		+ (colorTableLength * sizeof(RGBQUAD));

	auto bitmap = std::make_unique<Gdiplus::Bitmap>(bitmapInfo, data);

	if (bitmap->GetLastStatus() != Gdiplus::Ok)
	{
		return nullptr;
	}

	return GdiplusHelper::DeepCopyBitmap(bitmap.get());
}

wil::unique_hglobal WriteDIBDataToGlobal(Gdiplus::Bitmap *bitmap)
{
	Gdiplus::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	ScopedBitmapLock bitmapLock(bitmap, &rect, ScopedBitmapLock::LockMode::Read,
		PixelFormat32bppARGB);

	auto *bitmapData = bitmapLock.GetBitmapData();

	if (!bitmapData)
	{
		return nullptr;
	}

	size_t headerSize = sizeof(BITMAPINFOHEADER);
	size_t pixelDataSize = std::abs(bitmapData->Stride) * bitmapData->Height;
	size_t totalSize = headerSize + pixelDataSize;
	wil::unique_hglobal global(GlobalAlloc(GHND, totalSize));

	if (!global)
	{
		return nullptr;
	}

	wil::unique_hglobal_locked mem(global.get());

	if (!mem)
	{
		return nullptr;
	}

	// The CF_DIB format contains a BITMAPINFO structure, which is composed of a BITMAPINFOHEADER,
	// followed by the color space information and the bitmap bits. However, in this case, there is
	// no color space information, so the only parts present are the header and the bitmap bits.
	auto *infoHeader = static_cast<BITMAPINFOHEADER *>(mem.get());
	infoHeader->biSize = static_cast<DWORD>(headerSize);
	infoHeader->biWidth = bitmapData->Width;
	infoHeader->biHeight = bitmapData->Height;
	infoHeader->biPlanes = 1;
	infoHeader->biBitCount = 32;
	infoHeader->biCompression = BI_RGB;

	if (bitmapData->Stride > 0)
	{
		// If the stride is positive, this image is top-down. In the BITMAPINFOHEADER struct, a
		// top-down uncompressed image has a negative height.
		infoHeader->biHeight = -infoHeader->biHeight;
	}

	auto *pixelData = reinterpret_cast<std::byte *>(infoHeader) + headerSize;
	std::memcpy(pixelData, bitmapData->Scan0, pixelDataSize);

	return global;
}

bool IsDropFormatAvailable(IDataObject *dataObject, const FORMATETC &formatEtc)
{
	FORMATETC formatEtcCopy = formatEtc;
	HRESULT hr = dataObject->QueryGetData(&formatEtcCopy);
	return (hr == S_OK);
}

UINT GetPngClipboardFormat()
{
	// This is used by applications like Chrome when copying an image. The clipboard will contain
	// the raw png data.
	static UINT clipboardFormat = RegisterClipboardFormat(L"PNG");
	return clipboardFormat;
}

HRESULT MoveStorageToObject(IDataObject *dataObject, FORMATETC *format, wil::unique_stg_medium stg)
{
	RETURN_IF_FAILED(dataObject->SetData(format, &stg, true));

	// The IDataObject instance now owns the STGMEDIUM structure and is responsible for freeing the
	// memory associated with it.
	stg.release();

	return S_OK;
}
