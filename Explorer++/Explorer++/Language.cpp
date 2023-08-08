// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Helper.h"
#include "../Helper/ProcessHelper.h"

/*
 * Selects which language resource DLL based on user preferences and system language. The default
 * language is English.
 */
void Explorerplusplus::SetLanguageModule()
{
	HANDLE hFindFile;
	WIN32_FIND_DATA wfd;
	LANGID languageId;
	TCHAR szLanguageModule[MAX_PATH];
	TCHAR szNamePattern[MAX_PATH];
	TCHAR szFullFileName[MAX_PATH];
	TCHAR szName[MAX_PATH];
	WORD wLanguage;
	BOOL bRet;

	if (!m_commandLineSettings.language.empty())
	{
		/* Language has been forced on the command
		line by the user. Attempt to find the
		corresponding DLL. */
		GetProcessImageName(GetCurrentProcessId(), szLanguageModule,
			SIZEOF_ARRAY(szLanguageModule));
		PathRemoveFileSpec(szLanguageModule);
		StringCchPrintf(szName, SIZEOF_ARRAY(szName), _T("Explorer++%s.dll"),
			m_commandLineSettings.language.c_str());
		PathAppend(szLanguageModule, szName);

		bRet = GetFileLanguage(szLanguageModule, &wLanguage);

		if (bRet)
		{
			m_config->language = wLanguage;
		}
	}
	else
	{
		if (!m_bLanguageLoaded)
		{
			/* No previous language loaded. Try and use the system
			default language. */
			languageId = GetUserDefaultUILanguage();

			m_config->language = PRIMARYLANGID(languageId);
		}
	}

	if (m_config->language == LANG_ENGLISH)
	{
		m_resourceInstance = GetModuleHandle(nullptr);
	}
	else
	{
		GetProcessImageName(GetCurrentProcessId(), szLanguageModule,
			SIZEOF_ARRAY(szLanguageModule));
		PathRemoveFileSpec(szLanguageModule);

		StringCchCopy(szNamePattern, SIZEOF_ARRAY(szNamePattern), szLanguageModule);
		PathAppend(szNamePattern, NExplorerplusplus::LANGUAGE_DLL_FILENAME_PATTERN);

		hFindFile = FindFirstFile(szNamePattern, &wfd);

		/* Loop through the current translation DLL's to
		try and find one that matches the specified
		language. */
		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			StringCchCopy(szFullFileName, SIZEOF_ARRAY(szFullFileName), szLanguageModule);
			PathAppend(szFullFileName, wfd.cFileName);
			bRet = GetFileLanguage(szFullFileName, &wLanguage);

			BOOL bLanguageMismatch = FALSE;

			if (bRet && (wLanguage == m_config->language))
			{
				/* Using translation DLL's built for other versions of
				the executable will most likely crash the program due
				to incorrect/missing resources.
				Therefore, only load the specified translation DLL
				if it matches the current internal version. */
				if (VerifyLanguageVersion(szFullFileName))
				{
					m_resourceInstance = LoadLibrary(szFullFileName);
				}
				else
				{
					bLanguageMismatch = TRUE;
				}
			}
			else
			{
				while (FindNextFile(hFindFile, &wfd) != 0)
				{
					StringCchCopy(szFullFileName, SIZEOF_ARRAY(szFullFileName), szLanguageModule);
					PathAppend(szFullFileName, wfd.cFileName);
					bRet = GetFileLanguage(szFullFileName, &wLanguage);

					if (bRet && (wLanguage == m_config->language))
					{
						if (VerifyLanguageVersion(szFullFileName))
						{
							m_resourceInstance = LoadLibrary(szFullFileName);
						}
						else
						{
							bLanguageMismatch = TRUE;
						}

						break;
					}
				}
			}

			FindClose(hFindFile);

			if (bLanguageMismatch)
			{
				UINT stringId;

				/* Attempt to show an error message in the language
				that was specified. */
				switch (wLanguage)
				{
				case LANG_CHINESE_SIMPLIFIED:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_CHINESE_SIMPLIFIED;
					break;

				case LANG_CZECH:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_CZECH;
					break;

				case LANG_DANISH:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_DANISH;
					break;

				case LANG_DUTCH:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_DUTCH;
					break;

				case LANG_FRENCH:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_FRENCH;
					break;

				case LANG_GERMAN:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_GERMAN;
					break;

				case LANG_ITALIAN:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_ITALIAN;
					break;

				case LANG_JAPANESE:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_JAPANESE;
					break;

				case LANG_KOREAN:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_KOREAN;
					break;

				case LANG_NORWEGIAN:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_NORWEGIAN;
					break;

				case LANG_PORTUGUESE:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_PORTUGUESE;
					break;

				case LANG_ROMANIAN:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_ROMANIAN;
					break;

				case LANG_RUSSIAN:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_RUSSIAN;
					break;

				case LANG_SPANISH:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_SPANISH;
					break;

				default:
					stringId = IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH;
					break;
				}

				std::wstring versionMismatchMessage =
					ResourceHelper::LoadString(GetModuleHandle(nullptr), stringId);

				/* Main window hasn't been constructed yet, so this
				message box doesn't have any owner window. */
				MessageBox(nullptr, versionMismatchMessage.c_str(), NExplorerplusplus::APP_NAME,
					MB_ICONWARNING);
			}
		}
	}

	/* The language DLL was not found/could not be loaded.
	Use the default internal resource set. */
	if (m_resourceInstance == nullptr)
	{
		m_resourceInstance = GetModuleHandle(nullptr);

		m_config->language = LANG_ENGLISH;
	}
}

BOOL Explorerplusplus::VerifyLanguageVersion(const TCHAR *szLanguageModule) const
{
	TCHAR szImageName[MAX_PATH];
	DWORD dwpvProcessLS;
	DWORD dwpvProcessMS;
	DWORD dwpvLanguageLS;
	DWORD dwpvLanguageMS;
	DWORD dwRet;
	BOOL bSuccess1;
	BOOL bSuccess2;

	dwRet = GetProcessImageName(GetCurrentProcessId(), szImageName, SIZEOF_ARRAY(szImageName));

	if (dwRet != 0)
	{
		bSuccess1 = GetFileProductVersion(szImageName, &dwpvProcessLS, &dwpvProcessMS);
		bSuccess2 = GetFileProductVersion(szLanguageModule, &dwpvLanguageLS, &dwpvLanguageMS);

		if (bSuccess1 && bSuccess2)
		{
			return (dwpvLanguageMS == dwpvProcessMS) && (dwpvLanguageLS == dwpvProcessLS);
		}
	}

	return FALSE;
}
