// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>
#include <functional>
#include <list>
#include <string>

namespace RegistrySettings
{

LSTATUS SaveDword(HKEY key, const std::wstring &valueName, DWORD value);
LSTATUS ReadDword(HKEY key, const std::wstring &subKey, const std::wstring &valueName,
	DWORD &output);
LSTATUS ReadDword(HKEY key, const std::wstring &valueName, DWORD &output);
void ReadDword(HKEY key, const std::wstring &valueName,
	std::function<void(DWORD value)> successCallback);
LSTATUS SaveString(HKEY key, const std::wstring &valueName, const std::wstring &value);
LSTATUS ReadString(HKEY key, const std::wstring &valueName, std::wstring &output);
LSTATUS SaveStringList(HKEY key, const std::wstring &baseValueName,
	const std::list<std::wstring> &strings);
LSTATUS ReadStringList(HKEY key, const std::wstring &baseValueName,
	std::list<std::wstring> &outputStrings);
bool SaveDateTime(HKEY key, const std::wstring &baseValueName, const FILETIME &dateTime);
bool ReadDateTime(HKEY key, const std::wstring &baseValueName, FILETIME &outputDateTime);
LSTATUS SaveBinaryValue(HKEY key, const std::wstring &valueName, const BYTE *data, DWORD length);
LSTATUS ReadBinaryValueSize(HKEY key, const std::wstring &valueName, DWORD &length);
LSTATUS ReadBinaryValue(HKEY key, const std::wstring &valueName, void *data, DWORD length);

template <typename T>
LSTATUS Read32BitValueFromRegistry(HKEY key, const std::wstring &valueName, T &output)
{
	DWORD value;
	auto res = ReadDword(key, valueName, value);

	if (res == ERROR_SUCCESS)
	{
		output = value;
	}

	return res;
}

}
