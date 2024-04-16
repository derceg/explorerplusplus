// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/StringHelper.h"
#include "../Helper/Macros.h"
#include <gtest/gtest.h>
#include <tchar.h>

TEST(CheckWildcardMatch, SimpleMatches)
{
	EXPECT_EQ(CheckWildcardMatch(_T("*.txt"), _T("Test.txt"), TRUE), TRUE);
	EXPECT_EQ(CheckWildcardMatch(_T("?.txt"), _T("1.txt"), TRUE), TRUE);
	EXPECT_EQ(CheckWildcardMatch(_T("?ab*cd.tx?"), _T("1abefghcd.txt"), TRUE), TRUE);
	EXPECT_EQ(CheckWildcardMatch(_T("Test?1*txt"), _T("Test11test.txt"), TRUE), TRUE);
}

TEST(CheckWildcardMatch, UnicodeMatches)
{
/* Warning C4566 (character represented by universal-character-name
'char' cannot be represented in the current code page (page)) is given
here. Using an indirect string variable (rather than passing the
string directly to the function) seems to make the warning go away.
Possibly related to the bug raised at
http://connect.microsoft.com/VisualStudio/feedback/details/431433/unicode-strings-incorrectly-generating-c4566.
*/
#pragma warning(push)
#pragma warning(disable : 4566)

	EXPECT_EQ(CheckWildcardMatch(L"привет", L"Привет", FALSE), TRUE);
	EXPECT_EQ(CheckWildcardMatch(L"тестовую строку", L"ТЕСТОВУЮ СТРОКУ", FALSE), TRUE);
	EXPECT_EQ(CheckWildcardMatch(L"тестовую строку 2", L"ТЕСТОВУЮ СТРОКУ 2", TRUE), FALSE);
	EXPECT_EQ(CheckWildcardMatch(L"Тест?1*txt", L"Тест11Тест.txt", TRUE), TRUE);

#pragma warning(pop)
}

TEST(FormatSizeString, Simple)
{
	auto formattedSize = FormatSizeString(1);
	EXPECT_EQ(formattedSize, L"1 bytes");

	formattedSize = FormatSizeString(1024);
	EXPECT_EQ(formattedSize, L"1.00 KB");

	formattedSize = FormatSizeString(1024 * 1024 * 2);
	EXPECT_EQ(formattedSize, L"2.00 MB");

	formattedSize = FormatSizeString(48169402368);
	EXPECT_EQ(formattedSize, L"44.8 GB");

	formattedSize = FormatSizeString(517637815320);
	EXPECT_EQ(formattedSize, L"482 GB");

	formattedSize = FormatSizeString(1000202039296);
	EXPECT_EQ(formattedSize, L"931 GB");
}

TEST(ReplaceCharacter, NoMatches)
{
	TCHAR text[] = L"This is a test string";
	ReplaceCharacter(text, 'z', 'y');
	EXPECT_STREQ(text, L"This is a test string");
}

TEST(ReplaceCharacter, Simple)
{
	TCHAR text[] = L"This is a test string";
	ReplaceCharacter(text, 't', 'b');
	EXPECT_STREQ(text, L"This is a besb sbring");
}

TEST(ReplaceCharacterWithString, NoMatches)
{
	TCHAR outputText[64];
	ReplaceCharacterWithString(L"Hello world", outputText, SIZEOF_ARRAY(outputText), 'z',
		L"replacement");
	EXPECT_STREQ(outputText, L"Hello world");
}

TEST(ReplaceCharacterWithString, Simple)
{
	TCHAR outputText[64];
	ReplaceCharacterWithString(L"Hello world", outputText, SIZEOF_ARRAY(outputText), 'o',
		L"replacement");
	EXPECT_STREQ(outputText, L"Hellreplacement wreplacementrld");
}

TEST(TrimStringLeft, Simple)
{
	std::wstring text(L"  Test text");
	TrimStringLeft(text, L" ");
	EXPECT_EQ(text, L"Test text");
}

TEST(TrimStringRight, Simple)
{
	std::wstring text(L"Test text  ");
	TrimStringRight(text, L" ");
	EXPECT_EQ(text, L"Test text");
}

TEST(TrimString, Simple)
{
	std::wstring text(L"  Test text  ");
	TrimString(text, L" ");
	EXPECT_EQ(text, L"Test text");
}

TEST(WstrToStr, NullTerminator)
{
	// The resulting string shouldn't have a terminating NULL character embedded.
	auto result = WstrToStr(L"Test string");
	EXPECT_EQ(result, "Test string");
}

TEST(StrToWstr, NullTerminator)
{
	auto result = StrToWstr("Test string");
	EXPECT_EQ(result, L"Test string");
}
