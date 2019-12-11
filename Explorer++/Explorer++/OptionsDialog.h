// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Config.h"
#include "CoreInterface.h"
#include "TabContainer.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <wil/resource.h>

class OptionsDialog
{
public:

	static OptionsDialog *Create(std::shared_ptr<Config> config, HINSTANCE instance, IExplorerplusplus *expp,
		TabContainer *tabContainer);

	HWND Show(HWND parentWindow);

private:

	struct OptionsDialogSheetInfo
	{
		UINT resourceId;
		DLGPROC dlgProc;
	};

	static const OptionsDialogSheetInfo OPTIONS_DIALOG_SHEETS[];

	static const UINT_PTR PROP_SHEET_SUBCLASS_ID = 0;

	OptionsDialog(std::shared_ptr<Config> config, HINSTANCE instance, IExplorerplusplus *expp,
		TabContainer *tabContainer);
	~OptionsDialog() = default;

	PROPSHEETPAGE GeneratePropertySheetDefinition(const OptionsDialogSheetInfo &sheetInfo);

	static LRESULT CALLBACK PropSheetProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK PropSheetProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static INT_PTR CALLBACK GeneralSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK GeneralSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK FilesFoldersProcStub(HWND, UINT, WPARAM, LPARAM);
	INT_PTR CALLBACK FilesFoldersProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK WindowProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK WindowProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK TabSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK TabSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DefaultSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK DefaultSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/* Default settings dialog. */
	void OnDefaultSettingsNewTabDir(HWND hDlg);
	void DefaultSettingsSetNewTabDir(HWND hEdit, const TCHAR *szPath);
	void DefaultSettingsSetNewTabDir(HWND hEdit, PCIDLIST_ABSOLUTE pidl);

	/* Files and folders dialog. */
	void SetInfoTipWindowStates(HWND hDlg);
	void SetFolderSizeWindowState(HWND hDlg);

	void AddIconThemes(HWND dlg);
	void AddLanguages(HWND hDlg);
	BOOL AddLanguageToComboBox(HWND hComboBox, const TCHAR *szImageDirectory, const TCHAR *szFileName, WORD *pdwLanguage);
	int GetLanguageIDFromIndex(HWND hDlg, int iIndex);

	std::shared_ptr<Config> m_config;
	HINSTANCE m_instance;
	IExplorerplusplus *m_expp;
	DpiCompatibility m_dpiCompat;

	TabContainer *m_tabContainer;

	wil::unique_hicon m_optionsDialogIcon;
	wil::unique_hicon m_newTabDirectoryIcon;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
};