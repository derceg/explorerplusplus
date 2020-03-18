// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <tchar.h>
#include <string>
#include <list>

namespace NRegistrySettings
{
	LONG	SaveDwordToRegistry(HKEY hKey,const TCHAR *szKey,DWORD dwValue);
	LONG	ReadDwordFromRegistry(HKEY hKey,const TCHAR *szKey,DWORD *pReturnValue);
	LONG	SaveStringToRegistry(HKEY hKey,const TCHAR *szKey,const TCHAR *szValue);
	LONG	ReadStringFromRegistry(HKEY hKey,const TCHAR *szKey,TCHAR *szOutput,DWORD cchMax);
	LONG	ReadStringFromRegistry(HKEY hKey,const std::wstring &strKey,std::wstring &strOutput);
	LONG	SaveStringListToRegistry(HKEY hKey,const TCHAR *szBaseKeyName,const std::list<std::wstring> &strList);
	LONG	ReadStringListFromRegistry(HKEY hKey,const TCHAR *szBaseKeyName,std::list<std::wstring> &strList);
	bool	SaveDateTime(HKEY key, const std::wstring &baseKeyName, const FILETIME &dateTime);
	bool	ReadDateTime(HKEY key, const std::wstring &baseKeyName, FILETIME &dateTime);
}