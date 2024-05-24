// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DataExchangeHelper.h"
#include "GdiplusHelper.h"
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
	wil::unique_hglobal_locked mem(global);

	if (!mem)
	{
		return nullptr;
	}

	auto size = GlobalSize(mem.get());
	using StreamSizeType = UINT;

	if (size == 0 || size > (std::numeric_limits<StreamSizeType>::max)())
	{
		return nullptr;
	}

	// Note that SHCreateMemStream() is used instead of CreateStreamOnHGlobal(), as that function
	// requires that the backing HGLOBAL remains valid until the stream is destroyed, which doesn't
	// align with how this function works (the caller shouldn't have worry about the lifetime of the
	// input parameters after this function has returned). In this case, the Gdiplus::Bitmap
	// instance will still end up taking a reference on the stream, but that's ok, as the stream is
	// reference counted and is only initialized by the HGLOBAL, not backed by it.
	wil::com_ptr_nothrow<IStream> stream(
		SHCreateMemStream(static_cast<const BYTE *>(mem.get()), static_cast<StreamSizeType>(size)));

	if (!stream)
	{
		return nullptr;
	}

	auto bitmap = std::make_unique<Gdiplus::Bitmap>(stream.get());

	if (bitmap->GetLastStatus() != Gdiplus::Ok)
	{
		return nullptr;
	}

	return bitmap;
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
