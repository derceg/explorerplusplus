// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BetterEnumsWrapper.h"
#include <windows.h>
#include <functional>
#include <optional>
#include <string>

// clang-format off
BETTER_ENUM(SizeDisplayFormat, int,
	None = 0,
	Bytes = 1,
	KB = 2,
	MB = 3,
	GB = 4,
	TB = 5,
	PB = 6
)
// clang-format on

using StringComparatorFunc =
	std::function<bool(const std::wstring &input, const std::wstring &test)>;

[[nodiscard]] std::wstring FormatSizeString(uint64_t size,
	SizeDisplayFormat sizeDisplayFormat = SizeDisplayFormat::None);
BOOL CheckWildcardMatch(const TCHAR *szWildcard, const TCHAR *szString, BOOL bCaseSensitive);
void ReplaceCharacter(TCHAR *str, TCHAR ch, TCHAR chReplacement);
void ReplaceCharacterWithString(const TCHAR *szBaseString, TCHAR *szOutput, UINT cchMax,
	TCHAR chToReplace, const TCHAR *szReplacement);
void TrimStringLeft(std::wstring &str, const std::wstring &strWhitespace);
void TrimStringRight(std::wstring &str, const std::wstring &strWhitespace);
void TrimString(std::wstring &str, const std::wstring &strWhitespace);
std::optional<std::string> WstrToStr(const std::wstring &source);
std::optional<std::wstring> StrToWstr(const std::string &source);
std::string wstrToUtf8Str(const std::wstring &source);
std::wstring utf8StrToWstr(const std::string &source);
