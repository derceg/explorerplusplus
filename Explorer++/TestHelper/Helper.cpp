// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/Macros.h"
#include "Helper.h"

DWORD GetCurrentProcessImageName(TCHAR *szProcessPath, DWORD cchMax)
{
	return GetProcessImageName(GetCurrentProcessId(), szProcessPath, cchMax);
}

void GetTestResourceDirectory(TCHAR *szResourceDirectory, size_t cchMax)
{
	TCHAR szProcessPath[MAX_PATH];
	DWORD dwRet = GetCurrentProcessImageName(szProcessPath, SIZEOF_ARRAY(szProcessPath));
	ASSERT_NE(0, dwRet);

	BOOL bRet = PathRemoveFileSpec(szProcessPath);
	ASSERT_EQ(TRUE, bRet);

	ASSERT_EQ(MAX_PATH, cchMax);
	TCHAR *szRet = PathCombine(szResourceDirectory, szProcessPath, L"..\\TestResources");
	ASSERT_NE(nullptr, szRet);
}

void GetTestResourceDirectoryIdl(LPITEMIDLIST *pidl)
{
	TCHAR szResourceDirectory[MAX_PATH];
	GetTestResourceDirectory(szResourceDirectory, SIZEOF_ARRAY(szResourceDirectory));

	HRESULT hr = GetIdlFromParsingName(szResourceDirectory, pidl);
	ASSERT_TRUE(SUCCEEDED(hr));
}

void GetTestResourceFilePath(const TCHAR *szFile, TCHAR *szOutput, size_t cchMax)
{
	TCHAR szFullFileName[MAX_PATH];
	GetTestResourceDirectory(szFullFileName, SIZEOF_ARRAY(szFullFileName));

	BOOL bRet = PathAppend(szFullFileName, szFile);
	ASSERT_EQ(TRUE, bRet);

	HRESULT hr = StringCchCopy(szOutput, cchMax, szFullFileName);
	ASSERT_TRUE(SUCCEEDED(hr));
}

void GetTestResourceFileIdl(const TCHAR *szFile, LPITEMIDLIST *pidl)
{
	TCHAR szFullFileName[MAX_PATH];
	GetTestResourceFilePath(szFile, szFullFileName, SIZEOF_ARRAY(szFullFileName));

	HRESULT hr = GetIdlFromParsingName(szFullFileName, pidl);
	ASSERT_TRUE(SUCCEEDED(hr));
}