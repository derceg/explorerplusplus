// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <optional>

std::optional<std::wstring> ReadStringFromGlobal(HGLOBAL global);
wil::unique_hglobal WriteStringToGlobal(const std::wstring &str);
std::optional<std::string> ReadBinaryDataFromGlobal(HGLOBAL global);
wil::unique_hglobal WriteBinaryDataToGlobal(const std::string &data);

bool IsDropFormatAvailable(IDataObject *dataObject, const FORMATETC &formatEtc);
FORMATETC GetDroppedFilesFormatEtc();
std::vector<std::wstring> ExtractDroppedFilesList(IDataObject *dataObject);