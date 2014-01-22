#include "stdafx.h"
#include "gtest\gtest.h"
#include "../Helper/ProcessHelper.h"
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

void GetTestResourceFilePath(const TCHAR *szFile, TCHAR *szOutput, size_t cchMax)
{
	TCHAR szFullFileName[MAX_PATH];
	GetTestResourceDirectory(szFullFileName, SIZEOF_ARRAY(szFullFileName));

	BOOL bRet = PathAppend(szFullFileName, szFile);
	ASSERT_EQ(TRUE, bRet);

	HRESULT hr = StringCchCopy(szOutput, cchMax, szFullFileName);
	ASSERT_TRUE(SUCCEEDED(hr));
}