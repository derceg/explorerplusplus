// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LanguageHelper.h"
#include "CommandLine.h"
#include "Config.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"

namespace
{

// The language DLLs use the following naming scheme:
//
// Explorer++{language_code}.dll
//
// Where {language_code} is of the form:
//
// language[_country]
//
// and language is a 2 letter code.
//
// This function returns the expected language_code for a particular language.
//
// The list here should be kept in sync with the translation projects. When adding a translation
// project, an entry should also be added here.
std::optional<std::wstring> GetCodeForLanguage(LANGID language)
{
	switch (language)
	{
	case MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_UAE):
		return L"AR_AE";

	case MAKELANGID(LANG_CATALAN, SUBLANG_CATALAN_CATALAN):
		return L"CA";

	case MAKELANGID(LANG_CZECH, SUBLANG_CZECH_CZECH_REPUBLIC):
		return L"CS";

	case MAKELANGID(LANG_DANISH, SUBLANG_DANISH_DENMARK):
		return L"DA";

	case MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN):
		return L"DE";

	case MAKELANGID(LANG_GREEK, SUBLANG_GREEK_GREECE):
		return L"EL";

	case MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH):
		return L"ES";

	case MAKELANGID(LANG_PERSIAN, SUBLANG_PERSIAN_IRAN):
		return L"FA";

	case MAKELANGID(LANG_FINNISH, SUBLANG_FINNISH_FINLAND):
		return L"FI";

	case MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH):
		return L"FR";

	case MAKELANGID(LANG_HEBREW, SUBLANG_HEBREW_ISRAEL):
		return L"HE";

	case MAKELANGID(LANG_HUNGARIAN, SUBLANG_HUNGARIAN_HUNGARY):
		return L"HU";

	case MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN):
		return L"IT";

	case MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN):
		return L"JA";

	case MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN):
		return L"KO";

	case MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH):
		return L"NL";

	case MAKELANGID(LANG_NORWEGIAN, SUBLANG_DEFAULT):
		return L"NO";

	case MAKELANGID(LANG_POLISH, SUBLANG_POLISH_POLAND):
		return L"PL";

	case MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE):
		return L"PT";

	case MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN):
		return L"PT_BR";

	case MAKELANGID(LANG_ROMANIAN, SUBLANG_ROMANIAN_ROMANIA):
		return L"RO";

	case MAKELANGID(LANG_RUSSIAN, SUBLANG_RUSSIAN_RUSSIA):
		return L"RU";

	case MAKELANGID(LANG_SINHALESE, SUBLANG_SINHALESE_SRI_LANKA):
		return L"SI";

	case MAKELANGID(LANG_SWEDISH, SUBLANG_SWEDISH):
		return L"SV";

	case MAKELANGID(LANG_TURKISH, SUBLANG_TURKISH_TURKEY):
		return L"TR";

	case MAKELANGID(LANG_UKRAINIAN, SUBLANG_UKRAINIAN_UKRAINE):
		return L"UK";

	case MAKELANGID(LANG_VIETNAMESE, SUBLANG_VIETNAMESE_VIETNAM):
		return L"VI";

	case MAKELANGID(LANG_CHINESE_SIMPLIFIED, SUBLANG_CHINESE_SIMPLIFIED):
		return L"ZH_CN";

	case MAKELANGID(LANG_CHINESE_TRADITIONAL, SUBLANG_CHINESE_TRADITIONAL):
		return L"ZH_TW";
	}

	return std::nullopt;
}

bool VerifyLanguageVersion(const std::wstring &languageDllPath)
{
	wchar_t currentProcessPath[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), currentProcessPath,
		SIZEOF_ARRAY(currentProcessPath));

	DWORD processVersionLs;
	DWORD processVersionMs;
	auto res = GetFileProductVersion(currentProcessPath, &processVersionLs, &processVersionMs);

	if (!res)
	{
		return false;
	}

	DWORD languageDllVersionLs;
	DWORD languageDllVersionMs;
	res = GetFileProductVersion(languageDllPath.c_str(), &languageDllVersionLs,
		&languageDllVersionMs);

	if (!res)
	{
		return false;
	}

	return (languageDllVersionMs == processVersionMs) && (languageDllVersionLs == processVersionLs);
}

}

namespace LanguageHelper
{

std::variant<LanguageInfo, LoadError> MaybeLoadTranslationDll(
	const CommandLine::Settings *commandLineSettings, const Config *config)
{
	std::optional<std::wstring> languageCode;
	std::optional<LANGID> desiredLanguage;

	if (!commandLineSettings->language.empty())
	{
		languageCode = commandLineSettings->language;
	}
	else
	{
		languageCode = GetCodeForLanguage(config->language);
		desiredLanguage = config->language;
	}

	if (!languageCode)
	{
		return LoadError::LanguageCodeNotFound;
	}

	wchar_t currentProcessPath[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), currentProcessPath,
		SIZEOF_ARRAY(currentProcessPath));

	std::filesystem::path languageDllPath(currentProcessPath);
	languageDllPath.replace_filename(std::format(L"Explorer++{}.dll", *languageCode));

	WORD loadedLanguage;
	auto res = GetFileLanguage(languageDllPath.c_str(), &loadedLanguage);

	if (!res)
	{
		return LoadError::CouldntDetermineLanguage;
	}

	if (desiredLanguage && loadedLanguage != *desiredLanguage)
	{
		return LoadError::LanguageMismatch;
	}

	// Using a translation DLL built for a different version of the application will most likely
	// result in a crash, due to incorrect/missing resources. Therefore, only load the translation
	// DLL if it matches the current application version.
	auto sameVersion = VerifyLanguageVersion(languageDllPath.c_str());

	if (!sameVersion)
	{
		return LoadError::VersionMismatch;
	}

	auto resourceInstance = LoadLibrary(languageDllPath.c_str());

	if (!resourceInstance)
	{
		return LoadError::LoadFailed;
	}

	return LanguageInfo(loadedLanguage, resourceInstance);
}

bool IsLanguageRTL(LANGID language)
{
	LCID id = MAKELCID(language, SORT_DEFAULT);
	wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
	auto res = LCIDToLocaleName(id, localeName, std::size(localeName), 0);

	if (!res)
	{
		DCHECK(false);
		return false;
	}

	DWORD readingLayout;
	res = GetLocaleInfoEx(localeName, LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER,
		reinterpret_cast<wchar_t *>(&readingLayout), sizeof(readingLayout) / sizeof(wchar_t));

	if (!res)
	{
		DCHECK(false);
		return false;
	}

	// A value of 1 indicates an RTL language.
	return readingLayout == 1;
}

}
