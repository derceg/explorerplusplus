// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ProcessHelper.h"
#include "Helper.h"
#include <wil/resource.h>

DWORD GetProcessImageName(DWORD dwProcessId, TCHAR *szImageName, DWORD nSize)
{
	DWORD dwRet = 0;

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);

	if (hProcess != nullptr)
	{
		dwRet = GetModuleFileNameEx(hProcess, nullptr, szImageName, nSize);
		CloseHandle(hProcess);
	}

	return dwRet;
}

BOOL GetProcessOwner(DWORD dwProcessId, TCHAR *szOwner, size_t cchMax)
{
	BOOL success = FALSE;

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);

	if (hProcess != nullptr)
	{
		HANDLE hToken;
		BOOL bRet = OpenProcessToken(hProcess, TOKEN_QUERY, &hToken);

		if (bRet)
		{
			DWORD dwSize = 0;
			bRet = GetTokenInformation(hToken, TokenUser, nullptr, 0, &dwSize);

			if (!bRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				auto *pTokenUser = reinterpret_cast<TOKEN_USER *>(GlobalAlloc(GMEM_FIXED, dwSize));

				if (pTokenUser != nullptr)
				{
					bRet = GetTokenInformation(hToken, TokenUser,
						reinterpret_cast<LPVOID>(pTokenUser), dwSize, &dwSize);

					if (bRet)
					{
						success = FormatUserName(pTokenUser->User.Sid, szOwner, cchMax);
					}

					GlobalFree(pTokenUser);
				}
			}

			CloseHandle(hToken);
		}

		CloseHandle(hProcess);
	}

	return success;
}

BOOL SetProcessTokenPrivilege(DWORD dwProcessId, const TCHAR *PrivilegeName, BOOL bEnablePrivilege)
{
	BOOL success = FALSE;

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);

	if (hProcess != nullptr)
	{
		HANDLE hToken;
		BOOL bRet = OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken);

		if (bRet)
		{
			LUID luid;
			bRet = LookupPrivilegeValue(nullptr, PrivilegeName, &luid);

			if (bRet)
			{
				TOKEN_PRIVILEGES tp;
				tp.PrivilegeCount = 1;
				tp.Privileges[0].Luid = luid;

				if (bEnablePrivilege)
				{
					tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
				}
				else
				{
					tp.Privileges[0].Attributes = 0;
				}

				bRet = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, nullptr, nullptr);

				success = bRet && (GetLastError() == ERROR_SUCCESS);
			}

			CloseHandle(hToken);
		}

		CloseHandle(hProcess);
	}

	return success;
}

bool IsProcessElevated()
{
	wil::unique_handle token;
	BOOL res = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);

	if (!res)
	{
		return false;
	}

	TOKEN_ELEVATION tokenElevation;
	DWORD outputSize;
	res = GetTokenInformation(token.get(), TokenElevation, &tokenElevation, sizeof(tokenElevation),
		&outputSize);

	if (!res)
	{
		return false;
	}

	return tokenElevation.TokenIsElevated;
}
