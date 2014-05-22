/******************************************************************
*
* Project: Helper
* File: FileWrappers.cpp
* License: GPL - See LICENSE in the top level directory
*
* Provides unique_handle wrappers for file-related
* functions. These wrappers exist to ensure that
* function calls are mapped to the correct traits
* class.
*
* For example, some functions will return a NULL
* HANDLE on error, while others will return INVALID_HANDLE_VALUE,
* so it's important to make sure that each function
* is used with its correct traits implementation.
*
* This is especially important since supplying the
* wrong implementation will often result in the program
* silently doing the wrong thing, and this is
* particularly true for traits that manage the same
* base type.
*
* Written by David Erceg
* www.explorerplusplus.com
*
*****************************************************************/

#include "stdafx.h"
#include "FileWrappers.h"

HFilePtr CreateFilePtr(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	return HFilePtr(CreateFile(lpFileName, dwDesiredAccess, dwShareMode,
		lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
		hTemplateFile));
}

HFindFilePtr FindFirstFilePtr(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
	return HFindFilePtr(FindFirstFile(lpFileName, lpFindFileData));
}