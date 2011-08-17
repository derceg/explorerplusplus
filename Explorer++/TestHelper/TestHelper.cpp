#include "stdafx.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "gtest\gtest.h"

TEST(WildcardTest,SimpleMatches)
{
	EXPECT_EQ(TRUE,CheckWildcardMatch(_T("*.txt"),_T("Test.txt"),TRUE));
	EXPECT_EQ(TRUE,CheckWildcardMatch(_T("?.txt"),_T("1.txt"),TRUE));
	EXPECT_EQ(TRUE,CheckWildcardMatch(_T("?ab*cd.tx?"),_T("1abefghcd.txt"),TRUE));
	EXPECT_EQ(TRUE,CheckWildcardMatch(_T("Test?1*txt"),_T("Test11test.txt"),TRUE));
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

int _tmain(int argc, _TCHAR* argv[])
{
	::testing::InitGoogleTest(&argc,argv);
	return RUN_ALL_TESTS();
}