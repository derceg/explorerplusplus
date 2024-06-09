// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <gdiplus.h>
#include <optional>
#include <string>
#include <vector>

// Represents a virtual file, for use with CFSTR_FILEDESCRIPTOR.
struct VirtualFile
{
	std::wstring name;
	std::string contents;

	// This is only used in tests.
	bool operator==(const VirtualFile &) const = default;
};

std::optional<std::wstring> ReadStringFromGlobal(HGLOBAL global);
wil::unique_hglobal WriteStringToGlobal(const std::wstring &str);
std::optional<std::string> ReadBinaryDataFromGlobal(HGLOBAL global);
wil::unique_hglobal WriteBinaryDataToGlobal(const std::string &data);

wil::unique_hglobal WriteDataToGlobal(const void *data, size_t size);
std::optional<std::vector<std::wstring>> ReadHDropDataFromGlobal(HGLOBAL global);
wil::unique_hglobal WriteHDropDataToGlobal(const std::vector<std::wstring> &paths);
std::unique_ptr<Gdiplus::Bitmap> ReadPngDataFromGlobal(HGLOBAL global);
wil::unique_hglobal WritePngDataToGlobal(Gdiplus::Bitmap *bitmap);
std::unique_ptr<Gdiplus::Bitmap> ReadDIBDataFromGlobal(HGLOBAL global);
wil::unique_hglobal WriteDIBDataToGlobal(Gdiplus::Bitmap *bitmap);
HRESULT ReadVirtualFilesFromDataObject(IDataObject *dataObject,
	std::vector<VirtualFile> &virtualFilesOutput);
HRESULT WriteVirtualFilesToDataObject(IDataObject *dataObject,
	const std::vector<VirtualFile> &virtualFiles);

bool IsDropFormatAvailable(IDataObject *dataObject, const FORMATETC &formatEtc);
UINT GetPngClipboardFormat();
HRESULT MoveStorageToObject(IDataObject *dataObject, FORMATETC *format, wil::unique_stg_medium stg);
