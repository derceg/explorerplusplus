// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "StringHelper.h"
#include <codecvt>

BOOL CheckWildcardMatchInternal(const TCHAR *szWildcard, const TCHAR *szString,
	BOOL bCaseSensitive);

std::wstring FormatSizeString(uint64_t size, SizeDisplayFormat sizeDisplayFormat)
{
	static const TCHAR *SIZE_STRINGS[] = { _T("bytes"), _T("KB"), _T("MB"), _T("GB"), _T("TB"),
		_T("PB") };

	auto sizeAsDouble = static_cast<double>(size);
	size_t sizeIndex = 0;

	if (sizeDisplayFormat != +SizeDisplayFormat::None)
	{
		switch (sizeDisplayFormat)
		{
		case SizeDisplayFormat::Bytes:
			sizeIndex = 0;
			break;

		case SizeDisplayFormat::KB:
			sizeIndex = 1;
			break;

		case SizeDisplayFormat::MB:
			sizeIndex = 2;
			break;

		case SizeDisplayFormat::GB:
			sizeIndex = 3;
			break;

		case SizeDisplayFormat::TB:
			sizeIndex = 4;
			break;

		case SizeDisplayFormat::PB:
			sizeIndex = 5;
			break;

		case SizeDisplayFormat::None:
			break;
		}

		for (size_t i = 0; i < sizeIndex; i++)
		{
			sizeAsDouble /= 1024;
		}
	}
	else
	{
		while ((sizeAsDouble / 1024) >= 1)
		{
			sizeAsDouble /= 1024;

			sizeIndex++;
		}

		if (sizeIndex > (std::size(SIZE_STRINGS) - 1))
		{
			return {};
		}
	}

	int iPrecision;

	if (sizeIndex == 0)
	{
		iPrecision = 0;
	}
	else
	{
		if (sizeAsDouble < 10)
		{
			iPrecision = 2;
		}
		else if (sizeAsDouble < 100)
		{
			iPrecision = 1;
		}
		else
		{
			iPrecision = 0;
		}
	}

	int iLeast = static_cast<int>(
		(sizeAsDouble - static_cast<int>(sizeAsDouble)) * pow(10.0, iPrecision + 1));

	/* Setting the precision will cause automatic rounding. Therefore,
	if the least significant digit to be dropped is greater than 0.5,
	reduce it to below 0.5. */
	if (iLeast >= 5)
	{
		sizeAsDouble -= 5.0 * pow(10.0, -(iPrecision + 1));
	}

	std::wstringstream ss;
	ss.imbue(std::locale(""));
	ss.precision(iPrecision);

	ss << std::fixed << sizeAsDouble << _T(" ") << SIZE_STRINGS[sizeIndex];
	return ss.str();
}

BOOL CheckWildcardMatch(const TCHAR *szWildcard, const TCHAR *szString, BOOL bCaseSensitive)
{
	/* Handles multiple wildcard patterns. If the wildcard pattern contains ':',
	split the pattern into multiple subpatterns.
	For example "*.h: *.cpp" would match against "*.h" and "*.cpp" */
	BOOL bMultiplePattern = FALSE;

	for (int i = 0; i < lstrlen(szWildcard); i++)
	{
		if (szWildcard[i] == ':')
		{
			bMultiplePattern = TRUE;
			break;
		}
	}

	if (!bMultiplePattern)
	{
		return CheckWildcardMatchInternal(szWildcard, szString, bCaseSensitive);
	}
	else
	{
		TCHAR szWildcardPattern[512];
		TCHAR *szSinglePattern = nullptr;
		TCHAR *szSearchPattern = nullptr;
		TCHAR *szRemainingPattern = nullptr;

		StringCchCopy(szWildcardPattern, std::size(szWildcardPattern), szWildcard);

		szSinglePattern = wcstok_s(szWildcardPattern, _T(":"), &szRemainingPattern);
		PathRemoveBlanks(szSinglePattern);

		while (szSinglePattern != nullptr)
		{
			if (CheckWildcardMatchInternal(szSinglePattern, szString, bCaseSensitive))
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

	while (*szWildcard != '\0' && *szString != '\0' && bCurrentMatch)
	{
		switch (*szWildcard)
		{
			/* Match against the next part of the wildcard string.
			If there is a match, then return true, else consume
			the next character, and check again. */
		case '*':
			bMatched = FALSE;

			if (*(szWildcard + 1) != '\0')
			{
				bMatched = CheckWildcardMatch(++szWildcard, szString, bCaseSensitive);
			}

			while (*szWildcard != '\0' && *szString != '\0' && !bMatched)
			{
				/* Consume one more character on the input string,
				and keep (recursively) trying to match. */
				bMatched = CheckWildcardMatch(szWildcard, ++szString, bCaseSensitive);
			}

			if (bMatched)
			{
				while (*szWildcard != '\0')
				{
					szWildcard++;
				}

				szWildcard--;

				while (*szString != '\0')
				{
					szString++;
				}
			}

			bCurrentMatch = bMatched;
			break;

		case '?':
			szString++;
			break;

		default:
			if (bCaseSensitive)
			{
				bCurrentMatch = (*szWildcard == *szString);
			}
			else
			{
				TCHAR szCharacter1[1];
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, szWildcard, 1, szCharacter1,
					std::size(szCharacter1));

				TCHAR szCharacter2[1];
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, szString, 1, szCharacter2,
					std::size(szCharacter2));

				bCurrentMatch = (szCharacter1[0] == szCharacter2[0]);
			}

			szString++;
			break;
		}

		szWildcard++;
	}

	/* Skip past any trailing wildcards. */
	while (*szWildcard == '*')
	{
		szWildcard++;
	}

	if (*szWildcard == '\0' && *szString == '\0' && bCurrentMatch)
	{
		return TRUE;
	}

	return FALSE;
}

void ReplaceCharacter(TCHAR *str, TCHAR ch, TCHAR chReplacement)
{
	int i = 0;

	for (i = 0; i < lstrlen(str); i++)
	{
		if (str[i] == ch)
		{
			str[i] = chReplacement;
		}
	}
}

void ReplaceCharacterWithString(const TCHAR *szBaseString, TCHAR *szOutput, UINT cchMax,
	TCHAR chToReplace, const TCHAR *szReplacement)
{
	TCHAR szNewString[1024];
	int iBase = 0;
	int i = 0;

	szNewString[0] = '\0';
	for (i = 0; i < lstrlen(szBaseString); i++)
	{
		if (szBaseString[i] == chToReplace)
		{
			StringCchCatN(szNewString, std::size(szNewString), &szBaseString[iBase], i - iBase);
			StringCchCat(szNewString, std::size(szNewString), szReplacement);

			iBase = i + 1;
		}
	}

	StringCchCatN(szNewString, std::size(szNewString), &szBaseString[iBase], i - iBase);

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

std::optional<std::string> WstrToStr(const std::wstring &source)
{
	int length = WideCharToMultiByte(CP_ACP, 0, source.c_str(), -1, nullptr, 0, nullptr, nullptr);

	if (length == 0)
	{
		return std::nullopt;
	}

	std::string narrowString;
	narrowString.resize(length);

	length = WideCharToMultiByte(CP_ACP, 0, source.c_str(), -1, narrowString.data(),
		static_cast<int>(narrowString.size()), nullptr, nullptr);

	if (length == 0)
	{
		return std::nullopt;
	}

	narrowString.resize(length - 1);

	return narrowString;
}

std::optional<std::wstring> StrToWstr(const std::string &source)
{
	int length = MultiByteToWideChar(CP_ACP, 0, source.c_str(), -1, nullptr, 0);

	if (length == 0)
	{
		return std::nullopt;
	}

	std::wstring wideString;
	wideString.resize(length);

	length = MultiByteToWideChar(CP_ACP, 0, source.c_str(), -1, wideString.data(),
		static_cast<int>(wideString.size()));

	if (length == 0)
	{
		return std::nullopt;
	}

	wideString.resize(length - 1);

	return wideString;
}

// Generally speaking, the string returned by this function should only be used internally. Windows
// API functions, for example, will expect a different (non utf-8) narrow encoding.
std::string wstrToUtf8Str(const std::wstring &source)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(source);
}

std::wstring utf8StrToWstr(const std::string &source)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(source);
}
