// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "OptionsPage.h"
#include "../Helper/SetDefaultFileManager.h"
#include <wil/resource.h>
#include <memory>
#include <string>

class App;

class GeneralOptionsPage : public OptionsPage
{
public:
	GeneralOptionsPage(HWND parent, const ResourceLoader *resourceLoader, App *app, Config *config,
		CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback,
		HWND tooltipWindow);

	void SaveSettings() override;

private:
	std::unique_ptr<ResizableDialogHelper> InitializeResizeDialogHelper() override;
	void InitializeControls() override;
	void SetNewTabDirectory(HWND hEdit, const TCHAR *szPath);
	void SetNewTabDirectory(HWND hEdit, PCIDLIST_ABSOLUTE pidl);
	void AddLanguages();
	BOOL AddLanguageToComboBox(HWND hComboBox, const TCHAR *szImageDirectory,
		const TCHAR *szFileName, WORD *pdwLanguage);

	void OnCommand(WPARAM wParam, LPARAM lParam) override;
	void OnNewTabDirectoryButtonPressed();
	static int CALLBACK BrowseFolderCallbackStub(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData);
	void BrowseFolderCallback(HWND hwnd, UINT msg, LPARAM lParam);

	void OnReplaceExplorerSettingChanged(
		DefaultFileManager::ReplaceExplorerMode updatedReplaceMode);
	bool UpdateReplaceExplorerSetting(DefaultFileManager::ReplaceExplorerMode updatedReplaceMode);

	App *const m_app;
	wil::unique_hicon m_newTabDirectoryIcon;
	std::wstring m_newTabDirectory;
};
