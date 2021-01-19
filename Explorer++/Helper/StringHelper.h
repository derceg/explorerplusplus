// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>
#include <string>

enum class SizeDisplayFormat
{
	None = 0,
	Bytes = 1,
	KB = 2,
	MB = 3,
	GB = 4,
	TB = 5,
	PB = 6
};

void FormatSizeString(ULARGE_INTEGER lFileSize, TCHAR *pszFileSize, size_t cchBuf);
void FormatSizeString(ULARGE_INTEGER lFileSize, TCHAR *pszFileSize, size_t cchBuf, BOOL bForceSize,
	SizeDisplayFormat sdf);
TCHAR *PrintComma(unsigned long nPrint);
TCHAR *PrintCommaLargeNum(LARGE_INTEGER lPrint);
BOOL CheckWildcardMatch(const TCHAR *szWildcard, const TCHAR *szString, BOOL bCaseSensitive);
void ReplaceCharacter(TCHAR *str, TCHAR ch, TCHAR chReplacement);
void ReplaceCharacterWithString(const TCHAR *szBaseString, TCHAR *szOutput, UINT cchMax,
	TCHAR chToReplace, const TCHAR *szReplacement);
void TrimStringLeft(std::wstring &str, const std::wstring &strWhitespace);
void TrimStringRight(std::wstring &str, const std::wstring &strWhitespace);
void TrimString(std::wstring &str, const std::wstring &strWhitespace);
std::string wstrToUtf8Str(const std::wstring &source);
std::wstring utf8StrToWstr(const std::string &source);