// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DataExchangeHelper.h"

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
