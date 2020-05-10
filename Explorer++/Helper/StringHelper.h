// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>
#include <string>

enum SizeDisplayFormat_t
{
	SIZE_FORMAT_NONE,
	SIZE_FORMAT_BYTES,
	SIZE_FORMAT_KBYTES,
	SIZE_FORMAT_MBYTES,
	SIZE_FORMAT_GBYTES,
	SIZE_FORMAT_TBYTES,
	SIZE_FORMAT_PBYTES
};

void FormatSizeString(ULARGE_INTEGER lFileSize, TCHAR *pszFileSize, size_t cchBuf);
void FormatSizeString(ULARGE_INTEGER lFileSize, TCHAR *pszFileSize,
	size_t cchBuf, BOOL bForceSize, SizeDisplayFormat_t sdf);
TCHAR *PrintComma(unsigned long nPrint);
TCHAR *PrintCommaLargeNum(LARGE_INTEGER lPrint);
BOOL CheckWildcardMatch(const TCHAR *szWildcard, const TCHAR *szString, BOOL bCaseSensitive);
void ReplaceCharacter(TCHAR *str, TCHAR ch, TCHAR chReplacement);
void ReplaceCharacterWithString(const TCHAR *szBaseString, TCHAR *szOutput,
	UINT cchMax, TCHAR chToReplace, const TCHAR *szReplacement);
void TrimStringLeft(std::wstring &str, const std::wstring &strWhitespace);
void TrimStringRight(std::wstring &str, const std::wstring &strWhitespace);
void TrimString(std::wstring &str, const std::wstring &strWhitespace);
std::string wstrToStr(const std::wstring &source);
std::wstring strToWstr(const std::string &source);