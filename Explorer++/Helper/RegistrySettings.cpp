// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RegistrySettings.h"

namespace RegistrySettings
{

LSTATUS SaveDword(HKEY key, const std::wstring &valueName, DWORD value)
{
	return RegSetValueEx(key, valueName.c_str(), 0, REG_DWORD,
		reinterpret_cast<const BYTE *>(&value), sizeof(value));
}

// This could return something like:
//
// outcome::std_result<DWORD>
//
// but it would make the function more cumbersome to use in some cases. Which is why the output
// value is returned via a parameter.
//
// There are essentially two ways in which this function is used:
//
// 1. To read configuration values. In this case, it doesn't matter if the function fails and the
// output value can be left unchanged. Having to manually unwrap every value returned in an
// outcome::std_result would be cumbersome.
// 2. To read a specific set of items that all need to be successfully read. In this case,
// unwrapping the values isn't too big a deal, since the return values need to be checked.
//
// Since using an output parameter makes the first case simpler and also works for the second case,
// that's what's used here.
LSTATUS ReadDword(HKEY key, const std::wstring &subKey, const std::wstring &valueName,
	DWORD &output)
{
	DWORD value;
	DWORD size = sizeof(DWORD);

	LSTATUS res = RegGetValue(key, subKey.c_str(), valueName.c_str(), RRF_RT_REG_DWORD, nullptr,
		&value, &size);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	output = value;

	return res;
}

LSTATUS ReadDword(HKEY key, const std::wstring &valueName, DWORD &output)
{
	return ReadDword(key, L"", valueName, output);
}

void ReadDword(HKEY key, const std::wstring &valueName,
	std::function<void(DWORD value)> successCallback)
{
	DWORD value;
	LSTATUS result = ReadDword(key, valueName, value);

	if (result != ERROR_SUCCESS)
	{
		return;
	}

	successCallback(value);
}

LSTATUS SaveString(HKEY key, const std::wstring &valueName, const std::wstring &value)
{
	return RegSetValueEx(key, valueName.c_str(), 0, REG_SZ,
		reinterpret_cast<const BYTE *>(value.c_str()),
		static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
}

LSTATUS ReadString(HKEY key, const std::wstring &valueName, std::wstring &output)
{
	DWORD size;
	LSTATUS res =
		RegGetValue(key, nullptr, valueName.c_str(), RRF_RT_REG_SZ, nullptr, nullptr, &size);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	// size is in bytes and includes space for the terminating null character.
	std::wstring text;
	text.resize(size / sizeof(wchar_t));

	res = RegGetValue(key, nullptr, valueName.c_str(), RRF_RT_REG_SZ, nullptr, text.data(), &size);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	text.resize((size / sizeof(wchar_t)) - 1);

	output = text;

	return res;
}

// Saves a set of strings to the registry. Returns something other than ERROR_SUCCESS on failure. If
// this function does fail, any values that have been written will not be deleted (i.e. this
// function is not transactional).
LSTATUS SaveStringList(HKEY key, const std::wstring &baseValueName,
	const std::list<std::wstring> &strings)
{
	int i = 0;

	for (const auto &str : strings)
	{
		std::wstring itemValueName = std::format(L"{}{}", baseValueName, i++);

		LSTATUS res = SaveString(key, itemValueName, str);

		if (res != ERROR_SUCCESS)
		{
			return res;
		}
	}

	return ERROR_SUCCESS;
}

LSTATUS ReadStringList(HKEY key, const std::wstring &baseValueName,
	std::list<std::wstring> &outputStrings)
{
	LSTATUS res;
	int i = 0;

	do
	{
		std::wstring itemValueName = std::format(L"{}{}", baseValueName, i++);

		std::wstring value;
		res = ReadString(key, itemValueName, value);

		if (res == ERROR_SUCCESS)
		{
			outputStrings.push_back(value);
		}
	} while (res == ERROR_SUCCESS);

	// It is expected that the loop above will halt when the next key in the list doesn't exist. If
	// it halts for some other reason, then that's an error.
	if (res == ERROR_FILE_NOT_FOUND)
	{
		return ERROR_SUCCESS;
	}

	return res;
}

bool SaveDateTime(HKEY key, const std::wstring &baseValueName, const FILETIME &dateTime)
{
	auto res1 = SaveDword(key, baseValueName + L"Low", dateTime.dwLowDateTime);
	auto res2 = SaveDword(key, baseValueName + L"High", dateTime.dwHighDateTime);

	return (res1 == ERROR_SUCCESS && res2 == ERROR_SUCCESS);
}

bool ReadDateTime(HKEY key, const std::wstring &baseValueName, FILETIME &outputDateTime)
{
	auto res1 = ReadDword(key, baseValueName + L"Low", outputDateTime.dwLowDateTime);
	auto res2 = ReadDword(key, baseValueName + L"High", outputDateTime.dwHighDateTime);

	return (res1 == ERROR_SUCCESS && res2 == ERROR_SUCCESS);
}

LSTATUS SaveBinaryValue(HKEY key, const std::wstring &valueName, const BYTE *data, DWORD length)
{
	return RegSetValueEx(key, valueName.c_str(), 0, REG_BINARY, data, length);
}

LSTATUS ReadBinaryValueSize(HKEY key, const std::wstring &valueName, DWORD &length)
{
	return RegGetValue(key, nullptr, valueName.c_str(), RRF_RT_REG_BINARY, nullptr, nullptr,
		&length);
}

LSTATUS ReadBinaryValue(HKEY key, const std::wstring &valueName, void *data, DWORD length)
{
	DWORD outputLength = length;
	auto res = RegGetValue(key, nullptr, valueName.c_str(), RRF_RT_REG_BINARY, nullptr, data,
		&outputLength);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	if (outputLength != length)
	{
		return ERROR_BAD_LENGTH;
	}

	return res;
}

}
