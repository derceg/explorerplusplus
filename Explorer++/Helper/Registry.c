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


LONG SaveWindowPosition(HWND hwnd,TCHAR *RegistryPath)
{
	HKEY hKey;
	DWORD Disposition;
	LONG ReturnValue;
	BOOL bMaximized;
	WINDOWPLACEMENT wndpl;

	bMaximized = IsZoomed(hwnd);

	wndpl.length = sizeof(WINDOWPLACEMENT);

	GetWindowPlacement(hwnd,&wndpl);

	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,RegistryPath,0,NULL,
	REG_OPTION_NON_VOLATILE,KEY_WRITE,
	NULL,&hKey,&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey,_T("Left"),0,REG_DWORD,(LPBYTE)&wndpl.rcNormalPosition.left,sizeof(LONG));
		RegSetValueEx(hKey,_T("Top"),0,REG_DWORD,(LPBYTE)&wndpl.rcNormalPosition.top,sizeof(LONG));
		RegSetValueEx(hKey,_T("Right"),0,REG_DWORD,(LPBYTE)&wndpl.rcNormalPosition.right,sizeof(LONG));
		RegSetValueEx(hKey,_T("Bottom"),0,REG_DWORD,(LPBYTE)&wndpl.rcNormalPosition.bottom,sizeof(LONG));

		RegSetValueEx(hKey,_T("Maximized"),0,REG_DWORD,(LPBYTE)&bMaximized,sizeof(BOOL));
	}

	RegCloseKey(hKey);

	return ReturnValue;
}

LONG LoadWindowPosition(HWND hwnd,TCHAR *RegistryPath)
{
	HKEY hKey;
	LONG res;
	DWORD Type;
	DWORD SizeOfData;
	WINDOWPLACEMENT wndpl;
	BOOL bMaximized = FALSE;

	/* Create/Open the main registry key used to save the window position values. */
	res = RegOpenKeyEx(HKEY_CURRENT_USER,RegistryPath,0,KEY_READ,&hKey);

	/* Only attempt to save the values if the main key was opened (or created properly). */
	if(res == ERROR_SUCCESS)
	{
		/* Must indicate to the registry key function the size of the data that
		is being retrieved. */
		SizeOfData = sizeof(LONG);

		/* Load the window position values... */
		RegQueryValueEx(hKey,_T("Left"),0,&Type,(LPBYTE)&wndpl.rcNormalPosition.left,&SizeOfData);
		RegQueryValueEx(hKey,_T("Top"),0,&Type,(LPBYTE)&wndpl.rcNormalPosition.top,&SizeOfData);
		RegQueryValueEx(hKey,_T("Right"),0,&Type,(LPBYTE)&wndpl.rcNormalPosition.right,&SizeOfData);
		RegQueryValueEx(hKey,_T("Bottom"),0,&Type,(LPBYTE)&wndpl.rcNormalPosition.bottom,&SizeOfData);

		SizeOfData = sizeof(BOOL);
		RegQueryValueEx(hKey,_T("Maximized"),0,&Type,(LPBYTE)&bMaximized,&SizeOfData);

		wndpl.length	= sizeof(WINDOWPLACEMENT);
		wndpl.showCmd	= SW_HIDE;

		if(bMaximized)
			wndpl.showCmd |= SW_MAXIMIZE;

		SetWindowPlacement(hwnd,&wndpl);
	}

	RegCloseKey(hKey);

	return res;
}

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