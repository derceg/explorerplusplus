// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ShellHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wil/com.h>
#include <ShlObj.h>

using namespace testing;

void TestArePidlsEquivalent(const std::wstring &path1, const std::wstring &path2, bool equivalent);

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
	unique_pidl_absolute pidl;
	HRESULT hr = CreateSimplePidl(m_itemPath, wil::out_param(pidl), nullptr, GetParam());
	ASSERT_HRESULT_SUCCEEDED(hr);

	TestPidlProperties(pidl.get(), m_itemPath, m_itemName, GetParam());
}

TEST_P(CreateSimplePidlTest, Relative)
{
	unique_pidl_absolute pidlParent;
	HRESULT hr =
		CreateSimplePidl(m_parentPath, wil::out_param(pidlParent), nullptr, ShellItemType::Folder);
	ASSERT_HRESULT_SUCCEEDED(hr);

	wil::com_ptr_nothrow<IShellFolder> parent;
	hr = SHBindToObject(nullptr, pidlParent.get(), nullptr, IID_PPV_ARGS(&parent));
	ASSERT_HRESULT_SUCCEEDED(hr);

	unique_pidl_absolute pidl;
	hr = CreateSimplePidl(m_itemName, wil::out_param(pidl), parent.get(), GetParam());
	ASSERT_HRESULT_SUCCEEDED(hr);

	TestPidlProperties(pidl.get(), m_itemPath, m_itemName, GetParam());
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
	unique_pidl_absolute pidl;
	HRESULT hr = CreateSimplePidl(L"c:\\", wil::out_param(pidl));
	ASSERT_HRESULT_SUCCEEDED(hr);

	BOOL res = IsNamespaceRoot(pidl.get());
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

TEST(ArePidlsEquivalent, Same)
{
	TestArePidlsEquivalent(L"c:\\", L"c:\\", true);
	TestArePidlsEquivalent(L"c:\\users\\public", L"c:\\users\\public", true);
}

TEST(ArePidlsEquivalent, Different)
{
	TestArePidlsEquivalent(L"c:\\", L"c:\\windows", false);
	TestArePidlsEquivalent(L"c:\\", L"d:\\path\\to\\item", false);
}

void TestArePidlsEquivalent(const std::wstring &path1, const std::wstring &path2, bool equivalent)
{
	unique_pidl_absolute pidl1;
	HRESULT hr = CreateSimplePidl(path1, wil::out_param(pidl1));
	ASSERT_HRESULT_SUCCEEDED(hr);

	unique_pidl_absolute pidl2;
	hr = CreateSimplePidl(path2, wil::out_param(pidl2));
	ASSERT_HRESULT_SUCCEEDED(hr);

	BOOL res = ArePidlsEquivalent(pidl1.get(), pidl2.get());
	EXPECT_EQ(res, equivalent);
}

TEST(GetDisplayName, ParsingName)
{
	unique_pidl_absolute pidl;
	std::wstring pidlPath = L"c:\\path\\to\\file.txt";
	HRESULT hr = CreateSimplePidl(pidlPath, wil::out_param(pidl));
	ASSERT_HRESULT_SUCCEEDED(hr);

	std::wstring parsingName;
	hr = GetDisplayName(pidl.get(), SHGDN_FORPARSING, parsingName);
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_THAT(parsingName, StrCaseEq(pidlPath));
}
