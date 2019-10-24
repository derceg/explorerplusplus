// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "../Helper/ProcessHelper.h"

/*
* Selects which language resource DLL based
* on user preferences and system language.
* The default language is English.
*/
void Explorerplusplus::SetLanguageModule(void)
{
	HANDLE			hFindFile;
	WIN32_FIND_DATA	wfd;
	LANGID			LanguageID;
	TCHAR			szLanguageModule[MAX_PATH];
	TCHAR			szNamePattern[MAX_PATH];
	TCHAR			szFullFileName[MAX_PATH];
	TCHAR			szName[MAX_PATH];
	WORD			wLanguage;
	BOOL			bRet;

	if(g_bForceLanguageLoad)
	{
		/* Language has been forced on the command
		line by the user. Attempt to find the
		corresponding DLL. */
		GetProcessImageName(GetCurrentProcessId(), szLanguageModule, SIZEOF_ARRAY(szLanguageModule));
		PathRemoveFileSpec(szLanguageModule);
		StringCchPrintf(szName, SIZEOF_ARRAY(szName), _T("Explorer++%s.dll"), g_szLang);
		PathAppend(szLanguageModule, szName);

		bRet = GetFileLanguage(szLanguageModule, &wLanguage);

		if(bRet)
		{
			m_Language = wLanguage;
		}
		else
		{
			m_Language = LANG_ENGLISH;
		}
	}
	else
	{
		if(!m_bLanguageLoaded)
		{
			/* No previous language loaded. Try and use the system
			default language. */
			LanguageID = GetUserDefaultUILanguage();

			m_Language = PRIMARYLANGID(LanguageID);
		}
	}

	if(m_Language == LANG_ENGLISH)
	{
		m_hLanguageModule = GetModuleHandle(NULL);
	}
	else
	{
		GetProcessImageName(GetCurrentProcessId(), szLanguageModule, SIZEOF_ARRAY(szLanguageModule));
		PathRemoveFileSpec(szLanguageModule);

		StringCchCopy(szNamePattern, SIZEOF_ARRAY(szNamePattern), szLanguageModule);
		PathAppend(szNamePattern, NExplorerplusplus::LANGUAGE_DLL_FILENAME_PATTERN);

		hFindFile = FindFirstFile(szNamePattern, &wfd);

		/* Loop through the current translation DLL's to
		try and find one that matches the specified
		language. */
		if(hFindFile != INVALID_HANDLE_VALUE)
		{
			StringCchCopy(szFullFileName, SIZEOF_ARRAY(szFullFileName), szLanguageModule);
			PathAppend(szFullFileName, wfd.cFileName);
			bRet = GetFileLanguage(szFullFileName, &wLanguage);

			BOOL bLanguageMismatch = FALSE;

			if(bRet && (wLanguage == m_Language))
			{
				/* Using translation DLL's built for other versions of
				the executable will most likely crash the program due
				to incorrect/missing resources.
				Therefore, only load the specified translation DLL
				if it matches the current internal version. */
				if(VerifyLanguageVersion(szFullFileName))
				{
					m_hLanguageModule = LoadLibrary(szFullFileName);
				}
				else
				{
					bLanguageMismatch = TRUE;
				}
			}
			else
			{
				while(FindNextFile(hFindFile, &wfd) != 0)
				{
					StringCchCopy(szFullFileName, SIZEOF_ARRAY(szFullFileName), szLanguageModule);
					PathAppend(szFullFileName, wfd.cFileName);
					bRet = GetFileLanguage(szFullFileName, &wLanguage);

					if(bRet && (wLanguage == m_Language))
					{
						if(VerifyLanguageVersion(szFullFileName))
						{
							m_hLanguageModule = LoadLibrary(szFullFileName);
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

			if(bLanguageMismatch)
			{
				TCHAR szTemp[256];

				/* Attempt to show an error message in the language
				that was specified. */
				switch(wLanguage)
				{
				case LANG_CHINESE_SIMPLIFIED:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_CHINESE_SIMPLIFIED,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_CZECH:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_CZECH,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_DANISH:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_DANISH,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_DUTCH:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_DUTCH,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_FRENCH:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_FRENCH,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_GERMAN:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_GERMAN,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_ITALIAN:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_ITALIAN,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_JAPANESE:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_JAPANESE,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_KOREAN:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_KOREAN,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_NORWEGIAN:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_NORWEGIAN,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_PORTUGUESE:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_PORTUGUESE,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_ROMANIAN:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_ROMANIAN,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_RUSSIAN:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_RUSSIAN,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				case LANG_SPANISH:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH_SPANISH,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;

				default:
					LoadString(GetModuleHandle(NULL), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH,
						szTemp, SIZEOF_ARRAY(szTemp));
					break;
				}

				/* Main window hasn't been constructed yet, so this
				message box doesn't have any owner window. */
				MessageBox(NULL, szTemp, NExplorerplusplus::APP_NAME, MB_ICONWARNING);
			}
		}
	}

	/* The language DLL was not found/could not be loaded.
	Use the default internal resource set. */
	if(m_hLanguageModule == NULL)
	{
		m_hLanguageModule = GetModuleHandle(NULL);

		m_Language = LANG_ENGLISH;
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

	if(dwRet != 0)
	{
		bSuccess1 = GetFileProductVersion(szImageName, &dwpvProcessLS, &dwpvProcessMS);
		bSuccess2 = GetFileProductVersion(szLanguageModule, &dwpvLanguageLS, &dwpvLanguageMS);

		if(bSuccess1 && bSuccess2)
		{
			/* For the version of the language DLL to match
			the version of the executable, the major version,
			minor version and micro version must match. The
			build version is ignored. */
			if(HIWORD(dwpvLanguageMS) == HIWORD(dwpvProcessMS) &&
				LOWORD(dwpvLanguageMS) == LOWORD(dwpvProcessMS) &&
				HIWORD(dwpvLanguageLS) == HIWORD(dwpvProcessLS))
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}