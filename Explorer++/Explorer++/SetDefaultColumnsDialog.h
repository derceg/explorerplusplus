// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/FolderSettings.h"
#include "ThemedDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialogHelper.h"
#include <wil/resource.h>
#include <unordered_map>

enum class FolderType
{
	General = 0,
	Computer = 1,
	ControlPanel = 2,
	Network = 3,
	NetworkPlaces = 4,
	Printers = 5,
	RecycleBin = 6
};

class SetDefaultColumnsDialog;

class SetDefaultColumnsDialogPersistentSettings : public DialogSettings
{
public:
	static SetDefaultColumnsDialogPersistentSettings &GetInstance();

private:
	friend SetDefaultColumnsDialog;

	static const TCHAR SETTINGS_KEY[];

	static const TCHAR SETTING_FOLDER_TYPE[];

	SetDefaultColumnsDialogPersistentSettings();

	SetDefaultColumnsDialogPersistentSettings(const SetDefaultColumnsDialogPersistentSettings &);
	SetDefaultColumnsDialogPersistentSettings &operator=(
		const SetDefaultColumnsDialogPersistentSettings &);

	void SaveExtraRegistrySettings(HKEY hKey) override;
	void LoadExtraRegistrySettings(HKEY hKey) override;

	void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode) override;
	void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue) override;

	FolderType m_FolderType;
};

class SetDefaultColumnsDialog : public ThemedDialog
{
public:
	SetDefaultColumnsDialog(HINSTANCE resourceInstance, HWND hParent, ThemeManager *themeManager,
		FolderColumns &folderColumns);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *pnmhdr) override;
	INT_PTR OnClose() override;

private:
	std::vector<ResizableDialogControl> GetResizableControls() override;
	void SaveState() override;

	void OnOk();
	void OnCancel();
	void OnCbnSelChange();
	void OnLvnItemChanged(NMLISTVIEW *pnmlv);
	void OnMoveColumn(bool bUp);

	void SaveCurrentColumnState(FolderType folderType);
	void SetupFolderColumns(FolderType folderType);

	std::vector<Column_t> &GetCurrentColumnList(FolderType folderType);

	FolderColumns &m_folderColumns;

	std::unordered_map<int, FolderType> m_FolderMap;
	FolderType m_PreviousFolderType;

	wil::unique_hicon m_icon;

	SetDefaultColumnsDialogPersistentSettings *m_psdcdps;
};
