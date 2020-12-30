// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RegistrySettings.h"
#include "Macros.h"
#include <list>
#include <string>

LONG RegistrySettings::SaveDword(HKEY hKey, const TCHAR *valueName, DWORD dwValue)
{
	return RegSetValueEx(
		hKey, valueName, 0, REG_DWORD, reinterpret_cast<const BYTE *>(&dwValue), sizeof(dwValue));
}

LONG RegistrySettings::ReadDword(HKEY hKey, const TCHAR *valueName, DWORD *pReturnValue)
{
	DWORD dwSize = sizeof(DWORD);

	return RegQueryValueEx(
		hKey, valueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(pReturnValue), &dwSize);
}

LONG RegistrySettings::SaveString(HKEY hKey, const TCHAR *valueName, const TCHAR *szValue)
{
	return RegSetValueEx(hKey, valueName, 0, REG_SZ, reinterpret_cast<const BYTE *>(szValue),
		(lstrlen(szValue) + 1) * sizeof(TCHAR));
}

LONG RegistrySettings::ReadString(HKEY hKey, const TCHAR *valueName, TCHAR *szOutput, DWORD cchMax)
{
	LONG lRes;
	DWORD dwType;
	DWORD dwBufByteSize;
	DWORD dwBufChSize;

	dwBufByteSize = cchMax * sizeof(TCHAR);
	lRes = RegQueryValueEx(
		hKey, valueName, nullptr, &dwType, reinterpret_cast<LPBYTE>(szOutput), &dwBufByteSize);
	dwBufChSize = dwBufByteSize / sizeof(TCHAR);

	/* The returned buffer size includes any terminating
	NULL bytes (if the string was stored with a NULL byte).
	Therefore, if the string was stored with a NULL byte,
	the returned character String[dwBufChSize - 1] will be
	a NULL byte; if a string was not stored with a NULL byte,
	the size (and string) will not include any NULL bytes,
	and a NULL byte must be placed at String[dwBufChSize],
	providing that buffer size is smaller than the size
	of the incoming buffer. */
	if (dwBufChSize == 0 || dwType != REG_SZ)
	{
		szOutput[0] = '\0';
	}
	else
	{
		if (szOutput[dwBufChSize - 1] != '\0')
		{
			dwBufChSize = min(dwBufChSize, cchMax - 1);
			szOutput[dwBufChSize] = '\0';
		}
	}

	return lRes;
}

LONG RegistrySettings::ReadString(HKEY hKey, const std::wstring &valueName, std::wstring &strOutput)
{
	TCHAR szTemp[512];
	LONG lRes = RegistrySettings::ReadString(hKey, valueName.c_str(), szTemp, SIZEOF_ARRAY(szTemp));

	if (lRes == ERROR_SUCCESS)
	{
		strOutput = szTemp;
	}

	return lRes;
}

/* Saves a set of strings to the registry. Returns something other
than ERROR_SUCCESS on failure. If this function does fail, any values
that have been written will not be deleted (i.e. this function is
not transactional). */
LONG RegistrySettings::SaveStringList(
	HKEY hKey, const TCHAR *baseValueName, const std::list<std::wstring> &strList)
{
	TCHAR szItemKey[128];
	LONG lRes;
	int i = 0;

	for (const auto &str : strList)
	{
		StringCchPrintf(szItemKey, SIZEOF_ARRAY(szItemKey), _T("%s%d"), baseValueName, i++);
		lRes = SaveString(hKey, szItemKey, str.c_str());

		if (lRes != ERROR_SUCCESS)
		{
			return lRes;
		}
	}

	return ERROR_SUCCESS;
}

LONG RegistrySettings::ReadStringList(
	HKEY hKey, const TCHAR *baseValueName, std::list<std::wstring> &strList)
{
	TCHAR szItemKey[128];
	TCHAR szTemp[512];
	LONG lRes;
	int i = 0;

	do
	{
		StringCchPrintf(szItemKey, SIZEOF_ARRAY(szItemKey), _T("%s%d"), baseValueName, i++);

		lRes = ReadString(hKey, szItemKey, szTemp, SIZEOF_ARRAY(szTemp));

		if (lRes == ERROR_SUCCESS)
		{
			strList.emplace_back(szTemp);
		}
	} while (lRes == ERROR_SUCCESS);

	/* It is expected that the loop
	above will halt when the next
	key in the list doesn't exist.
	If it halts for some other
	reason (such as the buffer been
	to small), then that's an error. */
	if (lRes == ERROR_FILE_NOT_FOUND)
	{
		return ERROR_SUCCESS;
	}

	return lRes;
}

bool RegistrySettings::SaveDateTime(
	HKEY key, const std::wstring &baseValueName, const FILETIME &dateTime)
{
	LONG res1 = SaveDword(key, (baseValueName + L"Low").c_str(), dateTime.dwLowDateTime);
	LONG res2 = SaveDword(key, (baseValueName + L"High").c_str(), dateTime.dwHighDateTime);

	return (res1 == ERROR_SUCCESS && res2 == ERROR_SUCCESS);
}

bool RegistrySettings::ReadDateTime(HKEY key, const std::wstring &baseValueName, FILETIME &dateTime)
{
	LONG res1 = ReadDword(key, (baseValueName + L"Low").c_str(), &dateTime.dwLowDateTime);
	LONG res2 = ReadDword(key, (baseValueName + L"High").c_str(), &dateTime.dwHighDateTime);

	return (res1 == ERROR_SUCCESS && res2 == ERROR_SUCCESS);
}