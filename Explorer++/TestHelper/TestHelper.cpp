#include "stdafx.h"
#include "../Helper/Helper.h"
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

int _tmain(int argc, _TCHAR* argv[])
{
	::testing::InitGoogleTest(&argc,argv);
	return RUN_ALL_TESTS();
}