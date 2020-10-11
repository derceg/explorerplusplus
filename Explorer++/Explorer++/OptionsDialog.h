// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Config.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <wil/resource.h>

class DarkModeGroupBox;
__interface IExplorerplusplus;
class TabContainer;

class OptionsDialog
{
public:
	static OptionsDialog *Create(std::shared_ptr<Config> config, HINSTANCE instance,
		IExplorerplusplus *expp, TabContainer *tabContainer);

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

	static int CALLBACK PropertySheetCallback(HWND dialog, UINT msg, LPARAM lParam);
	static void OnPropertySheetInitialized(HWND dialog);

	static LRESULT CALLBACK PropSheetProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK PropSheetProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnDestroyDialog(HWND dlg);

	static INT_PTR CALLBACK GeneralSettingsProcStub(
		HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK GeneralSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK FilesFoldersProcStub(HWND, UINT, WPARAM, LPARAM);
	INT_PTR CALLBACK FilesFoldersProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK WindowProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK WindowProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK TabSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK TabSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DefaultSettingsProcStub(
		HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK DefaultSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	INT_PTR OnCtlColorDlg(HWND hwnd, HDC hdc);
	INT_PTR OnCtlColor(HWND hwnd, HDC hdc);
	INT_PTR OnCustomDraw(const NMCUSTOMDRAW *customDraw);

	void OnReplaceExplorerSettingChanged(
		HWND dialog, DefaultFileManager::ReplaceExplorerMode updatedReplaceMode);
	bool UpdateReplaceExplorerSetting(
		HWND dialog, DefaultFileManager::ReplaceExplorerMode updatedReplaceMode);

	/* Default settings dialog. */
	void OnDefaultSettingsNewTabDir(HWND hDlg);
	void DefaultSettingsSetNewTabDir(HWND hEdit, const TCHAR *szPath);
	void DefaultSettingsSetNewTabDir(HWND hEdit, PCIDLIST_ABSOLUTE pidl);

	/* Files and folders dialog. */
	void SetInfoTipWindowStates(HWND hDlg);
	void SetFolderSizeWindowState(HWND hDlg);

	void AddIconThemes(HWND dlg);
	void AddLanguages(HWND hDlg);
	BOOL AddLanguageToComboBox(
		HWND hComboBox, const TCHAR *szImageDirectory, const TCHAR *szFileName, WORD *pdwLanguage);
	int GetLanguageIDFromIndex(HWND hDlg, int iIndex);

	std::shared_ptr<Config> m_config;
	HINSTANCE m_instance;
	IExplorerplusplus *m_expp;

	TabContainer *m_tabContainer;

	wil::unique_hicon m_optionsDialogIcon;
	wil::unique_hicon m_newTabDirectoryIcon;

	std::unordered_set<int> m_checkboxControlIds;
	std::unordered_set<int> m_radioButtonControlIds;
	std::vector<std::unique_ptr<DarkModeGroupBox>> m_darkModeGroupBoxes;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;

	static inline int m_lastSelectedSheetIndex = 0;
};