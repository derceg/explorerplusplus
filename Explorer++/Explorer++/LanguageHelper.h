// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <variant>

namespace CommandLine
{

struct Settings;

}

struct Config;

namespace LanguageHelper
{

// This matches the language declaration within Explorer++.rc.
inline constexpr LANGID DEFAULT_LANGUAGE = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_AUS);

struct LanguageInfo
{
	LANGID language;
	HINSTANCE resourceInstance;
};

enum class LoadError
{
	LanguageCodeNotFound,
	CouldntDetermineLanguage,
	LanguageMismatch,
	VersionMismatch,
	LoadFailed
};

std::variant<LanguageInfo, LoadError> MaybeLoadTranslationDll(
	const CommandLine::Settings *commandLineSettings, const Config *config);
bool IsLanguageRTL(WORD primaryLanguageId);

}
