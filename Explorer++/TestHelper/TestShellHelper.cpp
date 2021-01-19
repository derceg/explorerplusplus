// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
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
	hr = BindToIdl(pidlResourceDirectory, IID_PPV_ARGS(&pShellFolder));
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

TEST(GetDisplayName, Simple)
{
	TCHAR szFullFileName[MAX_PATH];
	GetTestResourceFilePath(L"Metadata.jpg", szFullFileName, SIZEOF_ARRAY(szFullFileName));

	TCHAR szDisplayName[MAX_PATH];
	HRESULT hr = GetDisplayName(szFullFileName, szDisplayName, SIZEOF_ARRAY(szDisplayName), SHGDN_FORPARSING | SHGDN_INFOLDER);
	ASSERT_TRUE(SUCCEEDED(hr));

	EXPECT_STREQ(L"Metadata.jpg", szDisplayName);
}

TEST(IsPathGUID, GUID)
{
	BOOL bRet = IsPathGUID(L"::{26EE0668-A00A-44D7-9371-BEB064C98683}");
	EXPECT_EQ(TRUE, bRet);
}

TEST(IsPathGUID, NonGUID)
{
	TCHAR szResourceDirectory[MAX_PATH];
	GetTestResourceDirectory(szResourceDirectory, SIZEOF_ARRAY(szResourceDirectory));

	BOOL bRet = IsPathGUID(szResourceDirectory);
	EXPECT_EQ(FALSE, bRet);
}

void TestCompareIdls(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL bExpected)
{
	BOOL bRet = ArePidlsEquivalent(pidl1, pidl2);
	EXPECT_EQ(bExpected, bRet);
}

TEST(ArePidlsEquivalent, Same)
{
	LPITEMIDLIST pidl = NULL;
	GetTestResourceDirectoryIdl(&pidl);

	TestCompareIdls(pidl, pidl, TRUE);

	CoTaskMemFree(pidl);
}

TEST(ArePidlsEquivalent, Different)
{
	LPITEMIDLIST pidl1 = NULL;
	GetTestResourceFileIdl(L"Metadata.jpg", &pidl1);

	LPITEMIDLIST pidl2 = NULL;
	GetTestResourceFileIdl(L"ShellDetails.jpg", &pidl2);

	TestCompareIdls(pidl1, pidl2, FALSE);

	CoTaskMemFree(pidl1);
	CoTaskMemFree(pidl2);
}

TEST(GetItemAttributes, Simple)
{
	TCHAR szFullFileName[MAX_PATH];
	GetTestResourceFilePath(L"Metadata.jpg", szFullFileName, SIZEOF_ARRAY(szFullFileName));

	SFGAOF attributes;
	HRESULT hr = GetItemAttributes(szFullFileName, &attributes);
	ASSERT_TRUE(SUCCEEDED(hr));

	/* Since the exact list of
	attributes can vary by operating
	system version (i.e. newer
	versions of Windows can return
	newer properties), simply check
	that a few sample properties
	are present. */
	EXPECT_TRUE((attributes & SFGAO_CANCOPY) != 0);
	EXPECT_TRUE((attributes & SFGAO_FILESYSTEM) != 0);
	EXPECT_FALSE((attributes & SFGAO_FOLDER) != 0);
	EXPECT_FALSE((attributes & SFGAO_LINK) != 0);
}

TEST(IsNamespaceRoot, NonRoot)
{
	LPITEMIDLIST pidl = NULL;
	GetTestResourceDirectoryIdl(&pidl);

	BOOL bRet = IsNamespaceRoot(pidl);
	EXPECT_EQ(FALSE, bRet);

	CoTaskMemFree(pidl);
}

TEST(IsNamespaceRoot, Root)
{
	LPITEMIDLIST pidl = NULL;
	HRESULT hr = SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, &pidl);
	ASSERT_TRUE(SUCCEEDED(hr));

	BOOL bRet = IsNamespaceRoot(pidl);
	EXPECT_EQ(TRUE, bRet);

	CoTaskMemFree(pidl);
}