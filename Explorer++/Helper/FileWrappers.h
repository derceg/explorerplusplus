#pragma once

#include "UniqueHandle.h"

struct InvalidHandleTraits
{
	typedef HANDLE pointer;

	static HANDLE invalid()
	{
		return INVALID_HANDLE_VALUE;
	}

	static void close(HANDLE value)
	{
		CloseHandle(value);
	}
};

struct FindFileTraits
{
	typedef HANDLE pointer;

	static HANDLE invalid()
	{
		return INVALID_HANDLE_VALUE;
	}

	static void close(HANDLE value)
	{
		FindClose(value);
	}
};

typedef unique_handle<InvalidHandleTraits> HFilePtr;
typedef unique_handle<FindFileTraits> HFindFilePtr;

HFilePtr CreateFilePtr(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
HFindFilePtr FindFirstFilePtr(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);