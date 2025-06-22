// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ShellHelper.h"
#include "ShellTestHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wil/com.h>
#include <ShlObj.h>

using namespace testing;

class TransformPathTest : public Test
{
protected:
	void PerformTest(const std::wstring &userEnteredPath, const std::wstring &currentDirectory,
		const std::wstring &expectedPath,
		EnvVarsExpansion envVarsExpansionType = EnvVarsExpansion::Expand)
	{
		auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(userEnteredPath,
			currentDirectory, envVarsExpansionType);
		ASSERT_TRUE(absolutePath.has_value());
		EXPECT_EQ(absolutePath.value(), expectedPath);
	}
};

TEST_F(TransformPathTest, EnvironmentVariablesExpansion)
{
	std::wstring currentDirectory = L"c:\\windows";

	BOOL set = SetEnvironmentVariable(L"CustomDir", L"h:\\folder\\");
	ASSERT_TRUE(set);
	PerformTest(L"%CustomDir%", currentDirectory, L"h:\\folder\\");
	PerformTest(L"%CustomDir%", currentDirectory, L"c:\\windows\\%CustomDir%",
		EnvVarsExpansion::DontExpand);

	set = SetEnvironmentVariable(L"CustomFolderName", L"custom_folder_name");
	ASSERT_TRUE(set);
	PerformTest(L"c:\\%CustomFolderName%", currentDirectory, L"c:\\custom_folder_name");
	PerformTest(L"c:\\%CustomFolderName%", currentDirectory, L"c:\\%CustomFolderName%",
		EnvVarsExpansion::DontExpand);

	set = SetEnvironmentVariable(L"CustomFileName", L"custom_file_name.txt");
	ASSERT_TRUE(set);
	PerformTest(L"%CustomFileName%", currentDirectory, L"c:\\windows\\custom_file_name.txt");
	PerformTest(L"%CustomFileName%", currentDirectory, L"c:\\windows\\%CustomFileName%",
		EnvVarsExpansion::DontExpand);

	// Environment variables should be expanded consistently, regardless of the path they're
	// embedded within.
	set = SetEnvironmentVariable(L"DownloadsVar", L"downloads");
	ASSERT_TRUE(set);
	PerformTest(L"file:///c:/users/default/%DownloadsVar%", currentDirectory,
		L"c:\\users\\default\\downloads");
	PerformTest(L"shell:%DownloadsVar%", currentDirectory, L"shell:downloads");

	// Environment variables can contain arbitrary paths.
	set = SetEnvironmentVariable(L"FilePath", L"file:///c:/path/to/file");
	ASSERT_TRUE(set);
	PerformTest(L"%FilePath%", currentDirectory, L"c:\\path\\to\\file");

	set = SetEnvironmentVariable(L"ShellFolderPath", L"shell:public");
	ASSERT_TRUE(set);
	PerformTest(L"%ShellFolderPath%", currentDirectory, L"shell:public");
}

TEST_F(TransformPathTest, AbsolutePath)
{
	std::wstring currentDirectory = L"c:\\windows";

	// A file system path.
	PerformTest(L"c:\\users\\public", currentDirectory, L"c:\\users\\public");

	// A file system path with the long path prefix.
	PerformTest(L"\\\\?\\c:\\users\\public", currentDirectory, L"c:\\users\\public");

	// UNC paths.
	PerformTest(L"\\\\DESKTOP-ABCD\\shared-folder", currentDirectory,
		L"\\\\DESKTOP-ABCD\\shared-folder");
	PerformTest(L"\\\\?\\UNC\\DESKTOP-ABCD\\shared-folder", currentDirectory,
		L"\\\\DESKTOP-ABCD\\shared-folder");

	// The path of an item in the shell namespace.
	PerformTest(L"::{031E4825-7B94-4DC3-B131-E946B44C8DD5}", currentDirectory,
		L"::{031E4825-7B94-4DC3-B131-E946B44C8DD5}");

	// Absolute paths, anchored to the root of the current directory.
	PerformTest(L"\\", L"c:\\users\\public", L"c:\\");
	PerformTest(L"\\nested\\directory", L"d:\\path\\to\\item", L"d:\\nested\\directory");

	// file: URLs.
	PerformTest(L"file:///c:/users/", currentDirectory, L"c:\\users\\");
	PerformTest(L"file:///d:/path/to/file", currentDirectory, L"d:\\path\\to\\file");

	// A shell folder path.
	PerformTest(L"shell:downloads", currentDirectory, L"shell:downloads");

	// A search-ms: URL that represents a search results folder.
	PerformTest(
		L"search-ms:displayname=Search%20Results&crumb=fileextension%3A~<*.txt&crumb=location:C%3A%5CUsers%5CDefault",
		currentDirectory,
		L"search-ms:displayname=Search%20Results&crumb=fileextension%3A~<*.txt&crumb=location:C%3A%5CUsers%5CDefault");

	// An FTP path.
	PerformTest(L"ftp://127.0.0.1/", currentDirectory, L"ftp://127.0.0.1/");

	// Paths that are separated by forward slashes, rather than backslashes.
	PerformTest(L"c:/users/public", currentDirectory, L"c:\\users\\public");
	PerformTest(L"\\nested/directory", L"d:\\path\\to\\item", L"d:\\nested\\directory");
}

TEST_F(TransformPathTest, RelativePath)
{
	std::wstring currentDirectory = L"c:\\windows";

	PerformTest(L"system32", currentDirectory, L"c:\\windows\\system32");
	PerformTest(L"file.txt", currentDirectory, L"c:\\windows\\file.txt");
	PerformTest(L".\\path\\to\\nested\\directory", currentDirectory,
		L"c:\\windows\\path\\to\\nested\\directory");
	PerformTest(L"..\\users\\public", currentDirectory, L"c:\\users\\public");
	PerformTest(L"./system32/drivers", currentDirectory, L"c:\\windows\\system32\\drivers");
	PerformTest(L"../users/default", currentDirectory, L"c:\\users\\default");
}

TEST_F(TransformPathTest, Normalization)
{
	std::wstring currentDirectory = L"c:\\windows";

	PerformTest(L"c:\\path\\..\\", currentDirectory, L"c:\\");
	PerformTest(L"c:\\nested\\..\\path\\", currentDirectory, L"c:\\path\\");
	PerformTest(L"c:\\path\\.\\", currentDirectory, L"c:\\path\\");
	PerformTest(L".", currentDirectory, L"c:\\windows");
	PerformTest(L"..", currentDirectory, L"c:\\");
	PerformTest(L"file:///c:/users/..", currentDirectory, L"c:\\");

	// Paths expanded from environment variables should be normalized as well.
	BOOL set =
		SetEnvironmentVariable(L"VarWithRelativeReferences", L"d:\\path\\to\\nested\\..\\file");
	ASSERT_TRUE(set);
	PerformTest(L"%VarWithRelativeReferences%", currentDirectory, L"d:\\path\\to\\file");

	// It's not valid to try and perform normalization on a path like this. That is, this shouldn't
	// be transformed into "shell:public".
	PerformTest(L"shell:public\\subfolder\\..", currentDirectory, L"shell:public\\subfolder\\..");

	// Normalization shouldn't be performed on search-ms: URLs either.
	PerformTest(
		L"search-ms:displayname=Search%20Results&crumb=fileextension%3A~<*.txt&crumb=location:C%3A%5CUsers%5CDefault\\..",
		currentDirectory,
		L"search-ms:displayname=Search%20Results&crumb=fileextension%3A~<*.txt&crumb=location:C%3A%5CUsers%5CDefault\\..");

	PerformTest(L"ftp://127.0.0.1/directory/..", currentDirectory, L"ftp://127.0.0.1/directory/..");
}

TEST_F(TransformPathTest, Whitespace)
{
	std::wstring currentDirectory = L"c:\\windows";

	// Entering extraneous whitespace is explicitly allowed and should be ignored.
	PerformTest(L"        c:\\        ", currentDirectory, L"c:\\");
	PerformTest(L"  system32  ", currentDirectory, L"c:\\windows\\system32");
	PerformTest(L"    ..    ", currentDirectory, L"c:\\");

	// White space contained within expanded environment variables should also be ignored.
	BOOL set = SetEnvironmentVariable(L"CustomVar", L"    d:\\    ");
	ASSERT_TRUE(set);
	PerformTest(L"%CustomVar%", currentDirectory, L"d:\\");

	// Only ASCII space (0x20) characters should be ignored. Other whitespace characters should be
	// retained. \u2002 is En Space.
	PerformTest(L"c:\\users\\public\\file-with-trailing-whitespace\u2002", currentDirectory,
		L"c:\\users\\public\\file-with-trailing-whitespace\u2002");
}

// This class is essentially used to test that CreateSimplePidl returns a pidl with the correct set
// of properties. It's important the properties are set correctly, as other things (e.g. tests) rely
// on them.
class CreateSimplePidlTest : public TestWithParam<ShellItemType>
{
protected:
	CreateSimplePidlTest()
	{
		m_parentPath = L"c:\\path\\to";
		m_itemName = L"item";
		m_itemPath = m_parentPath + L"\\" + m_itemName;
	}

	static void TestPidlProperties(PCIDLIST_ABSOLUTE pidl, const std::wstring &itemPath,
		const std::wstring &itemName, ShellItemType shellItemType)
	{
		wil::com_ptr_nothrow<IShellItem> shellItem;
		HRESULT hr = SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&shellItem));
		ASSERT_HRESULT_SUCCEEDED(hr);

		wil::unique_cotaskmem_string displayName;
		hr = shellItem->GetDisplayName(SIGDN_NORMALDISPLAY, &displayName);
		ASSERT_HRESULT_SUCCEEDED(hr);
		EXPECT_EQ(displayName.get(), itemName);

		wil::unique_cotaskmem_string parsingPath;
		hr = shellItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &parsingPath);
		ASSERT_HRESULT_SUCCEEDED(hr);
		EXPECT_THAT(parsingPath.get(), StrCaseEq(itemPath));

		SFGAOF attributes;
		hr = shellItem->GetAttributes(SFGAO_FOLDER, &attributes);
		ASSERT_HRESULT_SUCCEEDED(hr);
		EXPECT_EQ(WI_IsFlagSet(attributes, SFGAO_FOLDER), shellItemType == ShellItemType::Folder);
	}

	std::wstring m_parentPath;
	std::wstring m_itemName;
	std::wstring m_itemPath;
};

TEST_P(CreateSimplePidlTest, Absolute)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(m_itemPath, nullptr, GetParam());
	TestPidlProperties(pidl.Raw(), m_itemPath, m_itemName, GetParam());
}

TEST_P(CreateSimplePidlTest, Relative)
{
	PidlAbsolute pidlParent = CreateSimplePidlForTest(m_parentPath);

	wil::com_ptr_nothrow<IShellFolder> parent;
	HRESULT hr = SHBindToObject(nullptr, pidlParent.Raw(), nullptr, IID_PPV_ARGS(&parent));
	ASSERT_HRESULT_SUCCEEDED(hr);

	PidlAbsolute pidl = CreateSimplePidlForTest(m_itemName, parent.get(), GetParam());
	TestPidlProperties(pidl.Raw(), m_itemPath, m_itemName, GetParam());
}

INSTANTIATE_TEST_SUITE_P(FileAndFolder, CreateSimplePidlTest,
	Values(ShellItemType::File, ShellItemType::Folder));

TEST(IsPathGUID, GUIDPath)
{
	bool res = IsPathGUID(L"::{26EE0668-A00A-44D7-9371-BEB064C98683}");
	EXPECT_TRUE(res);

	res = IsPathGUID(L"::{031E4825-7B94-4DC3-B131-E946B44C8DD5}\\Documents.library-ms");
	EXPECT_TRUE(res);
}

TEST(IsPathGUID, NonGUIDPath)
{
	bool res = IsPathGUID(L"c:\\");
	EXPECT_FALSE(res);

	res = IsPathGUID(L"\\DESKTOP-ABCD\\shared-folder");
	EXPECT_FALSE(res);
}

TEST(IsNamespaceRoot, NonRoot)
{
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\");

	BOOL res = IsNamespaceRoot(pidl.Raw());
	EXPECT_FALSE(res);
}

TEST(IsNamespaceRoot, Root)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, wil::out_param(pidl));
	ASSERT_HRESULT_SUCCEEDED(hr);

	BOOL res = IsNamespaceRoot(pidl.get());
	EXPECT_TRUE(res);
}

TEST(GetDisplayName, ParsingName)
{
	std::wstring pidlPath = L"c:\\path\\to\\file.txt";
	PidlAbsolute pidl = CreateSimplePidlForTest(pidlPath);

	std::wstring parsingName;
	HRESULT hr = GetDisplayName(pidl.Raw(), SHGDN_FORPARSING, parsingName);
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_THAT(parsingName, StrCaseEq(pidlPath));
}

class ExtractShellIconPartsTest : public Test
{
protected:
	void CheckExtraction(int iconIndex, int overlayIndex)
	{
		static_assert(sizeof(int) == 4);
		auto iconInfo = ExtractShellIconParts(iconIndex | (overlayIndex << 24));
		EXPECT_EQ(iconInfo.iconIndex, iconIndex);
		EXPECT_EQ(iconInfo.overlayIndex, overlayIndex);
	}
};

TEST_F(ExtractShellIconPartsTest, Extract)
{
	CheckExtraction(2, 0);
	CheckExtraction(53, 0);
	CheckExtraction(3, 1);
	CheckExtraction(21, 5);
}
