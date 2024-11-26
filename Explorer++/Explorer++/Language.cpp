// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "LanguageHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ResourceManager.h"
#include "Win32ResourceLoader.h"

void Explorerplusplus::SetLanguageModule()
{
	auto languageResult =
		LanguageHelper::MaybeLoadTranslationDll(m_app->GetCommandLineSettings(), m_config);
	LanguageHelper::LanguageInfo languageInfo;

	if (std::holds_alternative<LanguageHelper::LanguageInfo>(languageResult))
	{
		languageInfo = std::get<LanguageHelper::LanguageInfo>(languageResult);
	}
	else
	{
		auto errorCode = std::get<LanguageHelper::LoadError>(languageResult);

		if (errorCode == LanguageHelper::LoadError::VersionMismatch)
		{
			std::wstring versionMismatchMessage = ResourceHelper::LoadString(
				GetModuleHandle(nullptr), IDS_GENERAL_TRANSLATION_DLL_VERSION_MISMATCH);
			MessageBox(nullptr, versionMismatchMessage.c_str(), NExplorerplusplus::APP_NAME,
				MB_ICONWARNING);
		}

		languageInfo = { LanguageHelper::DEFAULT_LANGUAGE, GetModuleHandle(nullptr) };
	}

	m_config->language = languageInfo.language;
	m_resourceInstance = languageInfo.resourceInstance;

	if (LanguageHelper::IsLanguageRTL(PRIMARYLANGID(m_config->language)))
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
