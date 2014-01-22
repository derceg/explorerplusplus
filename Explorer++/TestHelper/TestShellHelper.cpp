#include "stdafx.h"
#include "gtest\gtest.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/Macros.h"
#include "Helper.h"

void TestGetShellItemDetailsEx(TCHAR *szFileName, const SHCOLUMNID *pscid, const TCHAR *szDetailExpected)
{
	TCHAR szResourceDirectory[MAX_PATH];
	GetTestResourceDirectory(szResourceDirectory, SIZEOF_ARRAY(szResourceDirectory));

	LPITEMIDLIST pidlResourceDirectory;
	HRESULT hr = GetIdlFromParsingName(szResourceDirectory, &pidlResourceDirectory);
	ASSERT_TRUE(SUCCEEDED(hr));

	IShellFolder2 *pShellFolder = NULL;
	hr = BindToIdl(pidlResourceDirectory, IID_IShellFolder2, reinterpret_cast<void **>(&pShellFolder));
	ASSERT_TRUE(SUCCEEDED(hr));

	LPITEMIDLIST pidlItem = NULL;
	hr = pShellFolder->ParseDisplayName(NULL, NULL, szFileName, NULL, &pidlItem, NULL);
	ASSERT_TRUE(SUCCEEDED(hr));

	TCHAR szDetail[512];
	hr = GetShellItemDetailsEx(pShellFolder, pscid, pidlItem, szDetail, SIZEOF_ARRAY(szDetail));
	CoTaskMemFree(pidlItem);
	ASSERT_TRUE(SUCCEEDED(hr));

	EXPECT_STREQ(szDetailExpected, szDetail);
}

TEST(GetShellItemDetailsEx, Author)
{
	TestGetShellItemDetailsEx(L"ShellDetails.jpg", &SCID_TITLE, L"Test title");
}

TEST(GetShellItemDetailsEx, Subject)
{
	TestGetShellItemDetailsEx(L"ShellDetails.jpg", &SCID_SUBJECT, L"Test subject");
}

TEST(GetShellItemDetailsEx, Comments)
{
	TestGetShellItemDetailsEx(L"ShellDetails.jpg", &SCID_COMMENTS, L"Test comments");
}