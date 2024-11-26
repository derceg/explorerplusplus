// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ResourceManager.h"
#include "Win32ResourceLoader.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"

namespace
{

bool IsLanguageRTL(WORD primaryLanguageId)
{
	return primaryLanguageId == LANG_ARABIC || primaryLanguageId == LANG_HEBREW;
}

}

/*
 * Selects which language resource DLL based on user preferences and system language. The default
 * language is English.
 */
void Explorerplusplus::SetLanguageModule()
{
	HANDLE hFindFile;
	WIN32_FIND_DATA wfd;
	TCHAR szLanguageModule[MAX_PATH];
	TCHAR szNamePattern[MAX_PATH];
	TCHAR szFullFileName[MAX_PATH];
	TCHAR szName[MAX_PATH];
	WORD wLanguage;
	BOOL bRet;

	if (!m_app->GetCommandLineSettings()->language.empty())
	{
		/* Language has been forced on the command
		line by the user. Attempt to find the
		corresponding DLL. */
		GetProcessImageName(GetCurrentProcessId(), szLanguageModule,
			SIZEOF_ARRAY(szLanguageModule));
		PathRemoveFileSpec(szLanguageModule);
		StringCchPrintf(szName, SIZEOF_ARRAY(szName), _T("Explorer++%s.dll"),
			m_app->GetCommandLineSettings()->language.c_str());
		PathAppend(szLanguageModule, szName);

		bRet = GetFileLanguage(szLanguageModule, &wLanguage);

		if (bRet)
		{
			m_config->language = wLanguage;
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

		StringCchCopy(szNamePattern, std::size(szNamePattern), szLanguageModule);
		PathAppend(szNamePattern, NExplorerplusplus::LANGUAGE_DLL_FILENAME_PATTERN);

		hFindFile = FindFirstFile(szNamePattern, &wfd);

		/* Loop through the current translation DLL's to
		try and find one that matches the specified
		language. */
		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			StringCchCopy(szFullFileName, std::size(szFullFileName), szLanguageModule);
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
					StringCchCopy(szFullFileName, std::size(szFullFileName), szLanguageModule);
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
				std::wstring versionMismatchMessage = ResourceHelper::LoadString(
					GetModuleHandle(nullptr), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH);

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

	if (IsLanguageRTL(PRIMARYLANGID(m_config->language)))
	{
		SetProcessDefaultLayout(LAYOUT_RTL);

		// TODO: As this function is currently called after the main window has been created,
		// SetProcessDefaultLayout() will have no effect on it. That's the reason why the
		// WS_EX_LAYOUTRTL style is manually applied here. If the language setting is loaded before
		// the main window is created, this will no longer be necessary.
		SetWindowLongPtr(m_hContainer, GWL_EXSTYLE,
			GetWindowLongPtr(m_hContainer, GWL_EXSTYLE) | WS_EX_LAYOUTRTL);
	}

	ResourceManager::Initialize(std::make_unique<Win32ResourceLoader>(m_resourceInstance));
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
