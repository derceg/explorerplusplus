/******************************************************************
 *
 * Project: Helper
 * File: Registry.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides various registry functionality.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Registry.h"


LONG SaveDwordToRegistry(HKEY hKey,TCHAR *KeyName,DWORD Value)
{
	return RegSetValueEx(hKey,KeyName,0,REG_DWORD,(LPBYTE)&Value,sizeof(Value));
}

LONG ReadDwordFromRegistry(HKEY hKey,TCHAR *KeyName,DWORD *pReturnValue)
{
	DWORD SizeOfData;

	SizeOfData = sizeof(DWORD);

	return RegQueryValueEx(hKey,KeyName,0,0,(LPBYTE)pReturnValue,&SizeOfData);
}

LONG SaveStringToRegistry(HKEY hKey,TCHAR *KeyName,TCHAR *String)
{
	return RegSetValueEx(hKey,KeyName,0,REG_SZ,(LPBYTE)String,lstrlen(String) * sizeof(TCHAR));
}

LONG ReadStringFromRegistry(HKEY hKey,TCHAR *KeyName,TCHAR *String,DWORD BufferSize)
{
	LONG	lRes;
	DWORD	dwType;
	DWORD	dwBufSize;

	dwBufSize = BufferSize;

	lRes = RegQueryValueEx(hKey,KeyName,0,&dwType,(LPBYTE)String,&dwBufSize);

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
		String[0] = '\0';
	}
	else
	{
		if(String[dwBufSize - 1] != '\0')
		{
			dwBufSize = min(dwBufSize,BufferSize);

			String[dwBufSize] = '\0';
		}
	}

	return lRes;
}