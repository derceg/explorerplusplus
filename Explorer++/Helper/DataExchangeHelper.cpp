// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DataExchangeHelper.h"

wil::unique_hglobal WriteDataToGlobal(const void *data, size_t size);

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

	return std::wstring(static_cast<const WCHAR *>(mem.get()), size / sizeof(WCHAR));
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

FORMATETC GetDroppedFilesFormatEtc()
{
	static FORMATETC formatEtc = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	return formatEtc;
}

std::vector<std::wstring> ExtractDroppedFilesList(IDataObject *dataObject)
{
	FORMATETC droppedFilesFormatEtc = GetDroppedFilesFormatEtc();
	wil::unique_stg_medium stgMedium;
	HRESULT hr = dataObject->GetData(&droppedFilesFormatEtc, &stgMedium);

	if (hr != S_OK)
	{
		return {};
	}

	wil::unique_hglobal_locked mem(stgMedium.hGlobal);

	if (!mem)
	{
		return {};
	}

	auto *dropFiles = static_cast<DROPFILES *>(mem.get());
	UINT numDroppedFiles =
		DragQueryFile(reinterpret_cast<HDROP>(dropFiles), 0xFFFFFFFF, nullptr, 0);
	std::vector<std::wstring> droppedFiles;

	for (UINT i = 0; i < numDroppedFiles; i++)
	{
		UINT numCharacters = DragQueryFile(reinterpret_cast<HDROP>(dropFiles), i, nullptr, 0);

		if (numCharacters == 0)
		{
			continue;
		}

		std::wstring fullFileName;
		fullFileName.resize(numCharacters + 1);

		UINT charactersCopied = DragQueryFile(reinterpret_cast<HDROP>(dropFiles), i,
			fullFileName.data(), static_cast<UINT>(fullFileName.capacity()));

		if (charactersCopied == 0)
		{
			continue;
		}

		droppedFiles.push_back(fullFileName);
	}

	return droppedFiles;
}