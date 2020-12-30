// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <tchar.h>
#include <string>
#include <list>

namespace NRegistrySettings
{
	LONG	SaveDwordToRegistry(HKEY hKey,const TCHAR *valueName,DWORD dwValue);
	LONG	ReadDwordFromRegistry(HKEY hKey,const TCHAR *valueName,DWORD *pReturnValue);
	LONG	SaveStringToRegistry(HKEY hKey,const TCHAR *valueName,const TCHAR *szValue);
	LONG	ReadStringFromRegistry(HKEY hKey,const TCHAR *szKey,TCHAR *valueName,DWORD cchMax);
	LONG	ReadStringFromRegistry(HKEY hKey,const std::wstring &valueName,std::wstring &strOutput);
	LONG	SaveStringListToRegistry(HKEY hKey,const TCHAR *baseValueName,const std::list<std::wstring> &strList);
	LONG	ReadStringListFromRegistry(HKEY hKey,const TCHAR *baseValueName,std::list<std::wstring> &strList);
	bool	SaveDateTime(HKEY key, const std::wstring &baseValueName, const FILETIME &dateTime);
	bool	ReadDateTime(HKEY key, const std::wstring &baseValueName, FILETIME &dateTime);

	template <typename T>
	LONG Read32BitValueFromRegistry(HKEY key, const std::wstring &valueName, T &output)
	{
		DWORD value;
		LONG result = ReadDwordFromRegistry(key, valueName.c_str(), &value);

		if (result == ERROR_SUCCESS)
		{
			output = value;
		}

		return result;
	}
}