/******************************************************************
 *
 * Project: Helper
 * File: RegistrySettings.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides various registry functionality.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include <string>
#include "RegistrySettings.h"


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
		lstrlen(szValue) * sizeof(TCHAR));
}

LONG NRegistrySettings::ReadStringFromRegistry(HKEY hKey,const TCHAR *szKey,TCHAR *szOutput,DWORD BufferSize)
{
	LONG	lRes;
	DWORD	dwType;
	DWORD	dwBufSize;

	dwBufSize = BufferSize;

	lRes = RegQueryValueEx(hKey,szKey,0,&dwType,reinterpret_cast<LPBYTE>(szOutput),&dwBufSize);

	/* The returned buffer size includes any terminating
	NULL bytes (if the string was stored with a NULL byte).
	Therefore, if the string was stored with a NULL byte,
	the returned character String[dwBufSize - 1] will be
	a NULL byte; if a string was not stored with a NULL byte,
	the size (and string) will not include any NULL bytes,
	and a NULL byte must be placed at String[dwBufSize],
	providing that buffer size is smaller than the size
	of the incoming buffer. */
	if(dwBufSize == 0 || dwType != REG_SZ)
	{
		szOutput[0] = '\0';
	}
	else
	{
		if(szOutput[dwBufSize - 1] != '\0')
		{
			dwBufSize = min(dwBufSize,BufferSize);
			szOutput[dwBufSize] = '\0';
		}
	}

	return lRes;
}

LONG NRegistrySettings::ReadStringFromRegistry(HKEY hKey,std::wstring strKey,std::wstring &strOutput)
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

	for each(auto str in strList)
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

	lRes = ERROR_SUCCESS;

	while(lRes == ERROR_SUCCESS)
	{
		StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
			_T("%s%d"),szBaseKeyName,i++);

		lRes = ReadStringFromRegistry(hKey,szItemKey,
			szTemp,SIZEOF_ARRAY(szTemp));

		if(lRes != ERROR_SUCCESS)
		{
			return lRes;
		}

		strList.push_back(szTemp);
	}

	return ERROR_SUCCESS;
}