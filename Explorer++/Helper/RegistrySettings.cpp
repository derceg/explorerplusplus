// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include <list>
#include <string>
#include "RegistrySettings.h"
#include "Macros.h"

LONG NRegistrySettings::SaveDwordToRegistry(HKEY hKey,const TCHAR *szKey,DWORD dwValue)
{
	return RegSetValueEx(hKey,szKey,0,REG_DWORD,reinterpret_cast<const BYTE *>(&dwValue),sizeof(dwValue));
}

LONG NRegistrySettings::ReadDwordFromRegistry(HKEY hKey,const TCHAR *szKey,DWORD *pReturnValue)
{
	DWORD dwSize = sizeof(DWORD);

	return RegQueryValueEx(hKey,szKey,0,0,reinterpret_cast<LPBYTE>(pReturnValue),&dwSize);
}

LONG NRegistrySettings::SaveStringToRegistry(HKEY hKey,const TCHAR *szKey,const TCHAR *szValue)
{
	return RegSetValueEx(hKey,szKey,0,REG_SZ,reinterpret_cast<const BYTE *>(szValue),
		(lstrlen(szValue) + 1) * sizeof(TCHAR));
}

LONG NRegistrySettings::ReadStringFromRegistry(HKEY hKey,const TCHAR *szKey,TCHAR *szOutput,DWORD cchMax)
{
	LONG	lRes;
	DWORD	dwType;
	DWORD	dwBufByteSize;
	DWORD	dwBufChSize;

	dwBufByteSize = cchMax * sizeof(TCHAR);
	lRes = RegQueryValueEx(hKey,szKey,0,&dwType,reinterpret_cast<LPBYTE>(szOutput),&dwBufByteSize);
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
	if(dwBufChSize == 0 || dwType != REG_SZ)
	{
		szOutput[0] = '\0';
	}
	else
	{
		if(szOutput[dwBufChSize - 1] != '\0')
		{
			dwBufChSize = min(dwBufChSize, cchMax - 1);
			szOutput[dwBufChSize] = '\0';
		}
	}

	return lRes;
}

LONG NRegistrySettings::ReadStringFromRegistry(HKEY hKey,const std::wstring &strKey,std::wstring &strOutput)
{
	TCHAR szTemp[512];
	LONG lRes = NRegistrySettings::ReadStringFromRegistry(hKey,strKey.c_str(),szTemp,SIZEOF_ARRAY(szTemp));

	if(lRes == ERROR_SUCCESS)
	{
		strOutput = szTemp;
	}

	return lRes;
}

/* Saves a set of strings to the registry. Returns something other
than ERROR_SUCCESS on failure. If this function does fail, any values
that have been written will not be deleted (i.e. this function is
not transactional). */
LONG NRegistrySettings::SaveStringListToRegistry(HKEY hKey,const TCHAR *szBaseKeyName,
	const std::list<std::wstring> &strList)
{
	TCHAR szItemKey[128];
	LONG lRes;
	int i = 0;

	for(const auto &str : strList)
	{
		StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),_T("%s%d"),
			szBaseKeyName,i++);
		lRes = SaveStringToRegistry(hKey,szItemKey,str.c_str());

		if(lRes != ERROR_SUCCESS)
		{
			return lRes;
		}
	}

	return ERROR_SUCCESS;
}

LONG NRegistrySettings::ReadStringListFromRegistry(HKEY hKey,const TCHAR *szBaseKeyName,
	std::list<std::wstring> &strList)
{
	TCHAR szItemKey[128];
	TCHAR szTemp[512];
	LONG lRes;
	int i = 0;

	do
	{
		StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
			_T("%s%d"),szBaseKeyName,i++);

		lRes = ReadStringFromRegistry(hKey,szItemKey,
			szTemp,SIZEOF_ARRAY(szTemp));

		if(lRes == ERROR_SUCCESS)
		{
			strList.emplace_back(szTemp);
		}
	} while(lRes == ERROR_SUCCESS);

	/* It is expected that the loop
	above will halt when the next
	key in the list doesn't exist.
	If it halts for some other
	reason (such as the buffer been
	to small), then that's an error. */
	if(lRes == ERROR_FILE_NOT_FOUND)
	{
		return ERROR_SUCCESS;
	}

	return lRes;
}

bool NRegistrySettings::SaveDateTime(HKEY key, const std::wstring &baseKeyName, const FILETIME &dateTime)
{
	LONG res1 = SaveDwordToRegistry(key, (baseKeyName + L"Low").c_str(), dateTime.dwLowDateTime);
	LONG res2 = SaveDwordToRegistry(key, (baseKeyName + L"High").c_str(), dateTime.dwHighDateTime);

	return (res1 == ERROR_SUCCESS && res2 == ERROR_SUCCESS);
}

bool NRegistrySettings::ReadDateTime(HKEY key, const std::wstring &baseKeyName, FILETIME &dateTime)
{
	LONG res1 = ReadDwordFromRegistry(key, (baseKeyName + L"Low").c_str(), &dateTime.dwLowDateTime);
	LONG res2 = ReadDwordFromRegistry(key, (baseKeyName + L"High").c_str(), &dateTime.dwHighDateTime);

	return (res1 == ERROR_SUCCESS && res2 == ERROR_SUCCESS);
}