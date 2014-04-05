#include "stdafx.h"
#include "../Helper/StringHelper.h"
#include "../Helper/Macros.h"

TEST(CheckWildcardMatch, SimpleMatches)
{
	EXPECT_EQ(TRUE, CheckWildcardMatch(_T("*.txt"), _T("Test.txt"), TRUE));
	EXPECT_EQ(TRUE, CheckWildcardMatch(_T("?.txt"), _T("1.txt"), TRUE));
	EXPECT_EQ(TRUE, CheckWildcardMatch(_T("?ab*cd.tx?"), _T("1abefghcd.txt"), TRUE));
	EXPECT_EQ(TRUE, CheckWildcardMatch(_T("Test?1*txt"), _T("Test11test.txt"), TRUE));
}

TEST(CheckWildcardMatch, UnicodeMatches)
{
	/* Warning C4566 (character represented by universal-character-name
	'char' cannot be represented in the current code page (page)) is given
	here. Using an indirect string variable (rather than passing the
	string directly to the function) seems to make the warning go away.
	Possibly related to the bug raised at
	http://connect.microsoft.com/VisualStudio/feedback/details/431433/unicode-strings-incorrectly-generating-c4566. */
	#pragma warning(push)
	#pragma warning(disable:4566)

	EXPECT_EQ(TRUE, CheckWildcardMatch(L"привет", L"Привет", FALSE));
	EXPECT_EQ(TRUE, CheckWildcardMatch(L"тестовую строку", L"ТЕСТОВУЮ СТРОКУ", FALSE));
	EXPECT_EQ(FALSE, CheckWildcardMatch(L"тестовую строку 2", L"ТЕСТОВУЮ СТРОКУ 2", TRUE));
	EXPECT_EQ(TRUE, CheckWildcardMatch(L"Тест?1*txt", L"Тест11Тест.txt", TRUE));

	#pragma warning(pop)
}

TEST(FormatSizeString, Simple)
{
	ULARGE_INTEGER ulSize;
	TCHAR szSize[128];

	ulSize.QuadPart = 1;
	FormatSizeString(ulSize, szSize, SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"1 bytes", szSize);

	ulSize.QuadPart = 1024;
	FormatSizeString(ulSize, szSize, SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"1.00 KB", szSize);

	ulSize.QuadPart = 1024 * 1024 * 2;
	FormatSizeString(ulSize, szSize, SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"2.00 MB", szSize);

	ulSize.QuadPart = 48169402368;
	FormatSizeString(ulSize, szSize, SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"44.8 GB", szSize);

	ulSize.QuadPart = 517637815320;
	FormatSizeString(ulSize, szSize, SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"482 GB", szSize);

	ulSize.QuadPart = 1000202039296;
	FormatSizeString(ulSize, szSize, SIZEOF_ARRAY(szSize));
	EXPECT_STREQ(L"931 GB", szSize);
}

TEST(PrintComma, Simple)
{
	TCHAR *szOut = PrintComma(1234567890);
	EXPECT_STREQ(L"1,234,567,890", szOut);
}

TEST(ReplaceCharacter, NoMatches)
{
	TCHAR szText[] = L"This is a test string";
	ReplaceCharacter(szText, 'z', 'y');
	EXPECT_STREQ(L"This is a test string", szText);
}

TEST(ReplaceCharacter, Simple)
{
	TCHAR szText[] = L"This is a test string";
	ReplaceCharacter(szText, 't', 'b');
	EXPECT_STREQ(L"This is a besb sbring", szText);
}

TEST(ReplaceCharacterWithString, NoMatches)
{
	TCHAR szOut[64];
	ReplaceCharacterWithString(L"Hello world", szOut, SIZEOF_ARRAY(szOut),
		'z', L"replacement");
	EXPECT_STREQ(L"Hello world", szOut);
}

TEST(ReplaceCharacterWithString, Simple)
{
	TCHAR szOut[64];
	ReplaceCharacterWithString(L"Hello world", szOut, SIZEOF_ARRAY(szOut),
		'o', L"replacement");
	EXPECT_STREQ(L"Hellreplacement wreplacementrld", szOut);
}

TEST(TrimStringLeft, Simple)
{
	std::wstring text(L"  Test text");
	TrimStringLeft(text, L" ");
	EXPECT_EQ(L"Test text", text);
}

TEST(TrimStringRight, Simple)
{
	std::wstring text(L"Test text  ");
	TrimStringRight(text, L" ");
	EXPECT_EQ(L"Test text", text);
}

TEST(TrimString, Simple)
{
	std::wstring text(L"  Test text  ");
	TrimString(text, L" ");
	EXPECT_EQ(L"Test text", text);
}