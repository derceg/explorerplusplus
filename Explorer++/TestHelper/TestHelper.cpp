#include "stdafx.h"
#include "../Helper/Helper.h"
#include "../Helper/StringHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/Macros.h"
#include "gtest\gtest.h"

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

DWORD GetCurrentProcessImageName(TCHAR *szProcessPath, DWORD cchMax)
{
	return GetProcessImageName(GetCurrentProcessId(), szProcessPath, cchMax);
}

void GetTestResourceFilePath(const TCHAR *szFile, TCHAR *szOutput, size_t cchMax)
{
	TCHAR szProcessPath[MAX_PATH];
	DWORD dwRet = GetCurrentProcessImageName(szProcessPath, SIZEOF_ARRAY(szProcessPath));
	ASSERT_NE(0, dwRet);

	BOOL bRet = PathRemoveFileSpec(szProcessPath);
	ASSERT_EQ(TRUE, bRet);

	TCHAR szFullFileName[MAX_PATH];
	TCHAR *szRet = PathCombine(szFullFileName, szProcessPath, L"..\\TestResources");
	ASSERT_NE(nullptr, szRet);

	bRet = PathAppend(szFullFileName, szFile);
	ASSERT_EQ(TRUE, bRet);

	StringCchCopy(szOutput, cchMax, szFullFileName);
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
	wLanguage = GetFileLanguage(szDLL);
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

int _tmain(int argc, _TCHAR* argv[])
{
	::testing::InitGoogleTest(&argc,argv);
	return RUN_ALL_TESTS();
}