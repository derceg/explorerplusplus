// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "StringHelper.h"
#include "Macros.h"
#include <codecvt>

BOOL CheckWildcardMatchInternal(const TCHAR *szWildcard, const TCHAR *szString, BOOL bCaseSensitive);

void FormatSizeString(ULARGE_INTEGER lFileSize, TCHAR *pszFileSize,
	size_t cchBuf)
{
	FormatSizeString(lFileSize, pszFileSize, cchBuf, FALSE, SIZE_FORMAT_NONE);
}

void FormatSizeString(ULARGE_INTEGER lFileSize, TCHAR *pszFileSize,
	size_t cchBuf, BOOL bForceSize, SizeDisplayFormat_t sdf)
{
	static const TCHAR *SIZE_STRINGS[] = {_T("bytes"), _T("KB"), _T("MB"), _T("GB"), _T("TB"), _T("PB")};

	auto fFileSize = static_cast<double>(lFileSize.QuadPart);
	int iSizeIndex = 0;

	if(bForceSize)
	{
		switch(sdf)
		{
		case SIZE_FORMAT_BYTES:
			iSizeIndex = 0;
			break;

		case SIZE_FORMAT_KBYTES:
			iSizeIndex = 1;
			break;

		case SIZE_FORMAT_MBYTES:
			iSizeIndex = 2;
			break;

		case SIZE_FORMAT_GBYTES:
			iSizeIndex = 3;
			break;

		case SIZE_FORMAT_TBYTES:
			iSizeIndex = 4;
			break;

		case SIZE_FORMAT_PBYTES:
			iSizeIndex = 5;
			break;
		}

		for(int i = 0; i < iSizeIndex; i++)
		{
			fFileSize /= 1024;
		}
	}
	else
	{
		while((fFileSize / 1024) >= 1)
		{
			fFileSize /= 1024;

			iSizeIndex++;
		}

		if(iSizeIndex > (SIZEOF_ARRAY(SIZE_STRINGS) - 1))
		{
			StringCchCopy(pszFileSize, cchBuf, EMPTY_STRING);
			return;
		}
	}

	int iPrecision;

	if(iSizeIndex == 0)
	{
		iPrecision = 0;
	}
	else
	{
		if(fFileSize < 10)
		{
			iPrecision = 2;
		}
		else if(fFileSize < 100)
		{
			iPrecision = 1;
		}
		else
		{
			iPrecision = 0;
		}
	}

	int iLeast = static_cast<int>((fFileSize - static_cast<int>(fFileSize)) *
		pow(10.0, iPrecision + 1));

	/* Setting the precision will cause automatic rounding. Therefore,
	if the least significant digit to be dropped is greater than 0.5,
	reduce it to below 0.5. */
	if(iLeast >= 5)
	{
		fFileSize -= 5.0 * pow(10.0, -(iPrecision + 1));
	}

	std::wstringstream ss;
	ss.imbue(std::locale(""));
	ss.precision(iPrecision);

	ss << std::fixed << fFileSize << _T(" ") << SIZE_STRINGS[iSizeIndex];
	std::wstring str = ss.str();
	StringCchCopy(pszFileSize, cchBuf, str.c_str());
}

TCHAR *PrintComma(unsigned long nPrint)
{
	LARGE_INTEGER lPrint;

	lPrint.LowPart = nPrint;
	lPrint.HighPart = 0;

	return PrintCommaLargeNum(lPrint);
}

TCHAR *PrintCommaLargeNum(LARGE_INTEGER lPrint)
{
	static TCHAR szBuffer[14];
	TCHAR *p = &szBuffer[SIZEOF_ARRAY(szBuffer) - 1];
	static TCHAR chComma = ',';
	auto nTemp = (unsigned long long)(lPrint.LowPart + (lPrint.HighPart * pow(2.0, 32.0)));
	int i = 0;

	if(nTemp == 0)
	{
		StringCchPrintf(szBuffer, SIZEOF_ARRAY(szBuffer), _T("%d"), 0);
		return szBuffer;
	}

	*p = (TCHAR)'\0';

	while(nTemp != 0)
	{
		if(i % 3 == 0 && i != 0)
			*--p = chComma;

		*--p = '0' + (TCHAR) (nTemp % 10);

		nTemp /= 10;

		i++;
	}

	return p;
}

BOOL CheckWildcardMatch(const TCHAR *szWildcard, const TCHAR *szString, BOOL bCaseSensitive)
{
	/* Handles multiple wildcard patterns. If the wildcard pattern contains ':',
	split the pattern into multiple subpatterns.
	For example "*.h: *.cpp" would match against "*.h" and "*.cpp" */
	BOOL bMultiplePattern = FALSE;

	for(int i = 0; i < lstrlen(szWildcard); i++)
	{
		if(szWildcard[i] == ':')
		{
			bMultiplePattern = TRUE;
			break;
		}
	}

	if(!bMultiplePattern)
	{
		return CheckWildcardMatchInternal(szWildcard, szString, bCaseSensitive);
	}
	else
	{
		TCHAR szWildcardPattern[512];
		TCHAR *szSinglePattern = NULL;
		TCHAR *szSearchPattern = NULL;
		TCHAR *szRemainingPattern = NULL;

		StringCchCopy(szWildcardPattern, SIZEOF_ARRAY(szWildcardPattern), szWildcard);

		szSinglePattern = wcstok_s(szWildcardPattern, _T(":"), &szRemainingPattern);
		PathRemoveBlanks(szSinglePattern);

		while(szSinglePattern != NULL)
		{
			if(CheckWildcardMatchInternal(szSinglePattern, szString, bCaseSensitive))
			{
				return TRUE;
			}

			szSearchPattern = szRemainingPattern;
			szSinglePattern = wcstok_s(szSearchPattern, _T(":"), &szRemainingPattern);
			PathRemoveBlanks(szSinglePattern);
		}
	}

	return FALSE;
}

BOOL CheckWildcardMatchInternal(const TCHAR *szWildcard, const TCHAR *szString, BOOL bCaseSensitive)
{
	BOOL bMatched;
	BOOL bCurrentMatch = TRUE;

	while(*szWildcard != '\0' && *szString != '\0' && bCurrentMatch)
	{
		switch(*szWildcard)
		{
			/* Match against the next part of the wildcard string.
			If there is a match, then return true, else consume
			the next character, and check again. */
		case '*':
			bMatched = FALSE;

			if(*(szWildcard + 1) != '\0')
			{
				bMatched = CheckWildcardMatch(++szWildcard, szString, bCaseSensitive);
			}

			while(*szWildcard != '\0' && *szString != '\0' && !bMatched)
			{
				/* Consume one more character on the input string,
				and keep (recursively) trying to match. */
				bMatched = CheckWildcardMatch(szWildcard, ++szString, bCaseSensitive);
			}

			if(bMatched)
			{
				while(*szWildcard != '\0')
					szWildcard++;

				szWildcard--;

				while(*szString != '\0')
					szString++;
			}

			bCurrentMatch = bMatched;
			break;

		case '?':
			szString++;
			break;

		default:
			if(bCaseSensitive)
			{
				bCurrentMatch = (*szWildcard == *szString);
			}
			else
			{
				TCHAR szCharacter1[1];
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, szWildcard, 1, szCharacter1, SIZEOF_ARRAY(szCharacter1));

				TCHAR szCharacter2[1];
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, szString, 1, szCharacter2, SIZEOF_ARRAY(szCharacter2));

				bCurrentMatch = (szCharacter1[0] == szCharacter2[0]);
			}

			szString++;
			break;
		}

		szWildcard++;
	}

	/* Skip past any trailing wildcards. */
	while(*szWildcard == '*')
		szWildcard++;

	if(*szWildcard == '\0' && *szString == '\0' && bCurrentMatch)
		return TRUE;

	return FALSE;
}

void ReplaceCharacter(TCHAR *str, TCHAR ch, TCHAR chReplacement)
{
	int  i = 0;

	for(i = 0; i < lstrlen(str); i++)
	{
		if(str[i] == ch)
		{
			str[i] = chReplacement;
		}
	}
}

void ReplaceCharacterWithString(const TCHAR *szBaseString, TCHAR *szOutput,
	UINT cchMax, TCHAR chToReplace, const TCHAR *szReplacement)
{
	TCHAR szNewString[1024];
	int iBase = 0;
	int i = 0;

	szNewString[0] = '\0';
	for(i = 0; i < lstrlen(szBaseString); i++)
	{
		if(szBaseString[i] == chToReplace)
		{
			StringCchCatN(szNewString, SIZEOF_ARRAY(szNewString),
				&szBaseString[iBase], i - iBase);
			StringCchCat(szNewString, SIZEOF_ARRAY(szNewString), szReplacement);

			iBase = i + 1;
		}
	}

	StringCchCatN(szNewString, SIZEOF_ARRAY(szNewString),
		&szBaseString[iBase], i - iBase);

	StringCchCopy(szOutput, cchMax, szNewString);
}

void TrimStringLeft(std::wstring &str, const std::wstring &strWhitespace)
{
	size_t pos = str.find_first_not_of(strWhitespace);
	str.erase(0, pos);
}

void TrimStringRight(std::wstring &str, const std::wstring &strWhitespace)
{
	size_t pos = str.find_last_not_of(strWhitespace);
	str.erase(pos + 1);
}

void TrimString(std::wstring &str, const std::wstring &strWhitespace)
{
	TrimStringLeft(str, strWhitespace);
	TrimStringRight(str, strWhitespace);
}

std::string wstrToStr(const std::wstring &source)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(source);
}

std::wstring strToWstr(const std::string &source)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(source);
}