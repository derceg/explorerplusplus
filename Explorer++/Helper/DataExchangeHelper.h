// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <gdiplus.h>
#include <optional>
#include <string>
#include <vector>

std::optional<std::wstring> ReadStringFromGlobal(HGLOBAL global);
wil::unique_hglobal WriteStringToGlobal(const std::wstring &str);
std::optional<std::string> ReadBinaryDataFromGlobal(HGLOBAL global);
wil::unique_hglobal WriteBinaryDataToGlobal(const std::string &data);

template <typename T>
	requires std::is_trivially_copyable_v<T> && std::is_trivially_constructible_v<T>
std::optional<T> ReadDataFromGlobal(HGLOBAL global)
{
	wil::unique_hglobal_locked mem(global);

	if (!mem)
	{
		return std::nullopt;
	}

	auto size = GlobalSize(mem.get());

	// As indicated by the documentation for GlobalSize(), the returned size can be larger than the
	// size requested when the memory was allocated. If the size is smaller than the size of the
	// target type, however, something has gone wrong and it doesn't make sense to try and use the
	// data.
	if (size < sizeof(T))
	{
		return std::nullopt;
	}

	return *static_cast<T *>(mem.get());
}

wil::unique_hglobal WriteDataToGlobal(const void *data, size_t size);
std::optional<std::vector<std::wstring>> ReadHDropDataFromGlobal(HGLOBAL global);
wil::unique_hglobal WriteHDropDataToGlobal(const std::vector<std::wstring> &paths);
std::unique_ptr<Gdiplus::Bitmap> ReadPngDataFromGlobal(HGLOBAL global);
wil::unique_hglobal WritePngDataToGlobal(Gdiplus::Bitmap *bitmap);
std::unique_ptr<Gdiplus::Bitmap> ReadDIBDataFromGlobal(HGLOBAL global);
wil::unique_hglobal WriteDIBDataToGlobal(Gdiplus::Bitmap *bitmap);

bool IsDropFormatAvailable(IDataObject *dataObject, const FORMATETC &formatEtc);
UINT GetPngClipboardFormat();
HRESULT MoveStorageToObject(IDataObject *dataObject, FORMATETC *format, wil::unique_stg_medium stg);
