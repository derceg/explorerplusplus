// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Config.h"
#include "DarkModeDialogBase.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <wil/resource.h>
#include <optional>
#include <unordered_map>

class CoreInterface;
class DarkModeGroupBox;
class TabContainer;

class OptionsDialog : public DarkModeDialogBase
{
public:
	OptionsDialog(HINSTANCE instance, HWND parent, std::shared_ptr<Config> config,
		CoreInterface *coreInterface, TabContainer *tabContainer);

private:
	struct PageInfo
	{
		UINT dialogResourceId;
		UINT titleResourceId;
		DLGPROC dialogProc;
	};

	enum class AdvancedOptionId
	{
		CheckSystemIsPinnedToNameSpaceTree,
		EnableDarkMode,
		OpenTabsInForeground
	};

	enum class AdvancedOptionType
	{
		Boolean
	};

	struct AdvancedOption
	{
		AdvancedOptionId id;
		std::wstring name;
		AdvancedOptionType type;
		std::wstring description;
	};

	static const PageInfo SETTINGS_PAGES[];

	// The amount of horizontal spacing between the treeview and each page.
	static constexpr int TREEVIEW_PAGE_HORIZONTAL_SPACING = 4;

	static constexpr UINT WM_APP_SAVE_SETTINGS = WM_APP + 1;

	INT_PTR OnInitDialog() override;
	wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;
	void AddSettingsPages();
	void AddSettingsPage(UINT dialogResourceId, UINT titleResourceId, int pageIndex,
		DLGPROC dialogProc, LPARAM dialogProcParam);
	void SelectPage(int index);

	INT_PTR OnNotify(NMHDR *nmhdr) override;
	void OnTreeViewSelectionChanged(const NMTREEVIEW *changeInfo);

	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	LRESULT HandleMenuOrAccelerator(WPARAM wParam);
	void OnOk();
	void OnApply();
	void OnCancel();
	INT_PTR OnClose() override;

	void OnSettingChanged();

	INT_PTR OnDestroy() override;
	INT_PTR OnNcDestroy() override;

	static INT_PTR CALLBACK GeneralSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam,
		LPARAM lParam);
	INT_PTR CALLBACK GeneralSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK FilesFoldersProcStub(HWND, UINT, WPARAM, LPARAM);
	INT_PTR CALLBACK FilesFoldersProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK WindowProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK WindowProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK TabSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK TabSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DefaultSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam,
		LPARAM lParam);
	INT_PTR CALLBACK DefaultSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK AdvancedSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam,
		LPARAM lParam);
	INT_PTR CALLBACK AdvancedSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	INT_PTR OnPageCtlColorDlg(HWND hwnd, HDC hdc);
	INT_PTR OnCtlColor(HWND hwnd, HDC hdc);
	INT_PTR OnCustomDraw(const NMCUSTOMDRAW *customDraw);

	void OnReplaceExplorerSettingChanged(HWND dialog,
		DefaultFileManager::ReplaceExplorerMode updatedReplaceMode);
	bool UpdateReplaceExplorerSetting(HWND dialog,
		DefaultFileManager::ReplaceExplorerMode updatedReplaceMode);

	/* Default settings dialog. */
	void OnDefaultSettingsNewTabDir(HWND hDlg);
	void DefaultSettingsSetNewTabDir(HWND hEdit, const TCHAR *szPath);
	void DefaultSettingsSetNewTabDir(HWND hEdit, PCIDLIST_ABSOLUTE pidl);

	/* Files and folders dialog. */
	void SetInfoTipWindowStates(HWND hDlg);
	void SetFolderSizeWindowState(HWND hDlg);

	void AddIconThemes(HWND dlg);
	void AddLanguages(HWND hDlg);
	BOOL AddLanguageToComboBox(HWND hComboBox, const TCHAR *szImageDirectory,
		const TCHAR *szFileName, WORD *pdwLanguage);
	int GetLanguageIDFromIndex(HWND hDlg, int iIndex);

	LRESULT CALLBACK AdvancedOptionsListViewWndProc(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam);
	std::vector<AdvancedOption> InitializeAdvancedOptions();
	void InsertAdvancedOptionsIntoListView(HWND dlg);
	bool GetBooleanConfigValue(AdvancedOptionId id);
	void SetBooleanConfigValue(AdvancedOptionId id, bool value);
	AdvancedOption *GetAdvancedOptionByIndex(HWND dlg, int index);

	std::shared_ptr<Config> m_config;
	HINSTANCE m_instance;
	CoreInterface *m_coreInterface;
	HWND m_tipWnd;

	std::unordered_map<int, HWND> m_dialogMap;
	std::unordered_map<int, HTREEITEM> m_treeMap;
	std::optional<int> m_currentPageIndex;
	bool m_initializationFinished = false;

	TabContainer *m_tabContainer;

	wil::unique_hicon m_optionsDialogIcon;
	wil::unique_hicon m_newTabDirectoryIcon;

	std::unordered_set<int> m_checkboxControlIds;
	std::unordered_set<int> m_radioButtonControlIds;
	std::vector<std::unique_ptr<DarkModeGroupBox>> m_darkModeGroupBoxes;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;

	std::vector<AdvancedOption> m_advancedOptions;
	std::unique_ptr<WindowSubclassWrapper> m_advancedOptionsListViewSubclass;

	static inline int m_lastSelectedPageIndex = 0;
};
