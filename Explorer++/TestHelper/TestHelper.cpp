#include "stdafx.h"
#include "gtest\gtest.h"
#include "../Helper/Helper.h"
#include "../Helper/StringHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Macros.h"
#include "Helper.h"

TEST(WildcardTest,SimpleMatches)
{
	EXPECT_EQ(TRUE,CheckWildcardMatch(_T("*.txt"),_T("Test.txt"),TRUE));
	EXPECT_EQ(TRUE,CheckWildcardMatch(_T("?.txt"),_T("1.txt"),TRUE));
	EXPECT_EQ(TRUE,CheckWildcardMatch(_T("?ab*cd.tx?"),_T("1abefghcd.txt"),TRUE));
	EXPECT_EQ(TRUE,CheckWildcardMatch(_T("Test?1*txt"),_T("Test11test.txt"),TRUE));
}

TEST(WildcardTest,UnicodeMatches)
{
	/* Warning C4566 (character represented by universal-character-name
	'char' cannot be represented in the current code page (page)) is given
	here. Using an indirect string variable (rather than passing the
	string directly to the function) seems to make the warning go away.
	Possibly related to the bug raised at
	http://connect.microsoft.com/VisualStudio/feedback/details/431433/unicode-strings-incorrectly-generating-c4566. */
	#pragma warning(push)
	#pragma warning(disable:4566)

	EXPECT_EQ(TRUE,CheckWildcardMatch(L"привет",L"Привет",FALSE));
	EXPECT_EQ(TRUE,CheckWildcardMatch(L"тестовую строку",L"ТЕСТОВУЮ СТРОКУ",FALSE));
	EXPECT_EQ(FALSE,CheckWildcardMatch(L"тестовую строку 2",L"ТЕСТОВУЮ СТРОКУ 2",TRUE));
	EXPECT_EQ(TRUE,CheckWildcardMatch(L"Тест?1*txt",L"Тест11Тест.txt",TRUE));

	#pragma warning(pop)
}

TEST(FormatSizeTest,Simple)
{
	ULARGE_INTEGER ulSize;
	TCHAR szSize[128];

	ulSize.QuadPart = 1;
	FormatSizeString(ulSize,szSize,SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"1 bytes",szSize);

	ulSize.QuadPart = 1024;
	FormatSizeString(ulSize,szSize,SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"1.00 KB",szSize);

	ulSize.QuadPart = 1024 * 1024 * 2;
	FormatSizeString(ulSize,szSize,SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"2.00 MB",szSize);

	ulSize.QuadPart = 48169402368;
	FormatSizeString(ulSize,szSize,SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"44.8 GB",szSize);

	ulSize.QuadPart = 517637815320;
	FormatSizeString(ulSize,szSize,SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"482 GB",szSize);

	ulSize.QuadPart = 1000202039296;
	FormatSizeString(ulSize,szSize,SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"931 GB",szSize);
}

void TestGetFileProductVersion(const TCHAR *szFile, DWORD dwExpectedLS, DWORD dwExpectedMS)
{
	TCHAR szDLL[MAX_PATH];
	GetTestResourceFilePath(szFile, szDLL, SIZEOF_ARRAY(szDLL));

	DWORD dwProductVersionLS;
	DWORD dwProductVersionMS;
	BOOL bRet = GetFileProductVersion(szDLL, &dwProductVersionLS, &dwProductVersionMS);
	ASSERT_EQ(TRUE, bRet);
	EXPECT_EQ(dwExpectedLS, dwProductVersionLS);
	EXPECT_EQ(dwExpectedMS, dwProductVersionMS);
}

TEST(GetFileProductVersion, Catalan)
{
	TestGetFileProductVersion(L"Explorer++CA.dll", 327680, 65539);
}

TEST(GetFileProductVersion, French)
{
	TestGetFileProductVersion(L"Explorer++FR.dll", 327684, 9);
}

void TestGetFileLanguage(const TCHAR *szFile, WORD wExpectedLanguage)
{
	TCHAR szDLL[MAX_PATH];
	GetTestResourceFilePath(szFile, szDLL, SIZEOF_ARRAY(szDLL));

	WORD wLanguage;
	BOOL bRet = GetFileLanguage(szDLL, &wLanguage);
	ASSERT_EQ(TRUE, bRet);
	EXPECT_EQ(wExpectedLanguage, wLanguage);
}

TEST(GetFileLanguage, Catalan)
{
	TestGetFileLanguage(L"Explorer++CA.dll", LANG_CATALAN);
}

TEST(GetFileLanguage, French)
{
	TestGetFileLanguage(L"Explorer++FR.dll", LANG_FRENCH);
}

void TestVersionInfoString(TCHAR *szDLL, const TCHAR *szVersionInfo, const TCHAR *szExpectedValue)
{
	/* Somewhat flaky, as it relies
	on the users default language
	been English. */
	TCHAR szOutput[512];
	BOOL bRet = GetVersionInfoString(szDLL, szVersionInfo, szOutput, SIZEOF_ARRAY(szOutput));
	ASSERT_EQ(TRUE, bRet);
	EXPECT_STREQ(szExpectedValue, szOutput);
}

TEST(GetVersionInfoString, Simple)
{
	TCHAR szDLL[MAX_PATH];
	GetTestResourceFilePath(L"VersionInfo.dll", szDLL, SIZEOF_ARRAY(szDLL));

	TestVersionInfoString(szDLL, L"CompanyName", L"Test company");
	TestVersionInfoString(szDLL, L"FileDescription", L"Test file description");
	TestVersionInfoString(szDLL, L"FileVersion", L"1.18.3.3624");
	TestVersionInfoString(szDLL, L"InternalName", L"VersionI.dll");
	TestVersionInfoString(szDLL, L"LegalCopyright", L"Copyright (C) 2014");
	TestVersionInfoString(szDLL, L"OriginalFilename", L"VersionI.dll");
	TestVersionInfoString(szDLL, L"ProductName", L"Test product name");
	TestVersionInfoString(szDLL, L"ProductVersion", L"1.18.23.4728");
}

class HardLinkTest : public ::testing::Test
{
public:

	HardLinkTest() : m_bDeleteDirectory(FALSE)
	{

	}

protected:

	void SetUp()
	{
		GetTestResourceDirectory(m_szResourceDirectory, SIZEOF_ARRAY(m_szResourceDirectory));
		BOOL bRet = PathAppend(m_szResourceDirectory, L"HardLinkTemp");
		ASSERT_NE(FALSE, bRet);

		bRet = CreateDirectory(m_szResourceDirectory, NULL);
		ASSERT_NE(FALSE, bRet);

		m_bDeleteDirectory = TRUE;
	}

	void TearDown()
	{
		if(m_bDeleteDirectory)
		{
			std::list<std::wstring> fileList;
			fileList.push_back(m_szResourceDirectory);
			BOOL bRet = NFileOperations::DeleteFiles(NULL, fileList, TRUE, TRUE);
			ASSERT_EQ(TRUE, bRet);
		}
	}

	void CreateTestFileHardLink(const TCHAR *szOriginalFile, const TCHAR *szNewFileName)
	{
		TCHAR szCopy[MAX_PATH];
		TCHAR *szRet = PathCombine(szCopy, m_szResourceDirectory, szNewFileName);
		ASSERT_NE(nullptr, szRet);

		BOOL bRet = CreateHardLink(szCopy, szOriginalFile, NULL);
		ASSERT_NE(FALSE, bRet);
	}

	TCHAR m_szResourceDirectory[MAX_PATH];

private:

	BOOL m_bDeleteDirectory;
};

TEST_F(HardLinkTest, Simple)
{
	TCHAR szRoot[MAX_PATH];
	StringCchCopy(szRoot, SIZEOF_ARRAY(szRoot), m_szResourceDirectory);
	BOOL bRet = PathStripToRoot(szRoot);
	ASSERT_NE(FALSE, bRet);

	TCHAR szName[32];
	bRet = GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, NULL, szName, SIZEOF_ARRAY(szName));
	ASSERT_NE(FALSE, bRet);

	/* A little hacky, but hard links are
	only supported on NTFS. */
	if(lstrcmp(L"NTFS", szName) != 0)
	{
		return;
	}

	TCHAR szOriginalFile[MAX_PATH];
	GetTestResourceFilePath(L"VersionInfo.dll", szOriginalFile, SIZEOF_ARRAY(szOriginalFile));

	CreateTestFileHardLink(szOriginalFile, L"HardLinkCopy1");
	CreateTestFileHardLink(szOriginalFile, L"HardLinkCopy2");
	CreateTestFileHardLink(szOriginalFile, L"HardLinkCopy3");

	/* 4 hard links expected - 3 copies + the original. */
	DWORD dwLinks = GetNumFileHardLinks(szOriginalFile);
	EXPECT_EQ(4, dwLinks);
}

void TestReadImageProperty(const TCHAR *szFile, PROPID propId, const TCHAR *szPropertyExpected)
{
	TCHAR szFullFileName[MAX_PATH];
	GetTestResourceFilePath(szFile, szFullFileName, SIZEOF_ARRAY(szFullFileName));

	TCHAR szProperty[512];
	BOOL bRes = ReadImageProperty(szFullFileName, propId, szProperty, SIZEOF_ARRAY(szProperty));
	ASSERT_EQ(TRUE, bRes);

	EXPECT_STREQ(szPropertyExpected, szProperty);
}

TEST(ReadImageProperty, EquipMake)
{
	TestReadImageProperty(L"Metadata.jpg", PropertyTagEquipMake, L"Test camera maker");
}

TEST(ReadImageProperty, EquipModel)
{
	TestReadImageProperty(L"Metadata.jpg", PropertyTagEquipModel, L"Test camera model");
}