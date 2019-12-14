// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ShellBrowser.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"
#include <wil/resource.h>
#include <unordered_map>

enum FolderType_t
{
	FOLDER_TYPE_GENERAL = 0,
	FOLDER_TYPE_COMPUTER = 1,
	FOLDER_TYPE_CONTROL_PANEL = 2,
	FOLDER_TYPE_NETWORK = 3,
	FOLDER_TYPE_NETWORK_PLACES = 4,
	FOLDER_TYPE_PRINTERS = 5,
	FOLDER_TYPE_RECYCLE_BIN = 6
};

class CSetDefaultColumnsDialog;

class CSetDefaultColumnsDialogPersistentSettings : public CDialogSettings
{
public:

	static CSetDefaultColumnsDialogPersistentSettings &GetInstance();

private:

	friend CSetDefaultColumnsDialog;

	static const TCHAR SETTINGS_KEY[];

	static const TCHAR SETTING_FOLDER_TYPE[];

	CSetDefaultColumnsDialogPersistentSettings();

	CSetDefaultColumnsDialogPersistentSettings(const CSetDefaultColumnsDialogPersistentSettings &);
	CSetDefaultColumnsDialogPersistentSettings & operator=(const CSetDefaultColumnsDialogPersistentSettings &);

	void			SaveExtraRegistrySettings(HKEY hKey);
	void			LoadExtraRegistrySettings(HKEY hKey);

	void			SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode);
	void			LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue);

	FolderType_t	m_FolderType;
};

class CSetDefaultColumnsDialog : public CBaseDialog
{
public:

	CSetDefaultColumnsDialog(HINSTANCE hInstance, HWND hParent, FolderColumns &folderColumns);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnNotify(NMHDR *pnmhdr);
	INT_PTR	OnClose();

private:

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);
	void	SaveState();

	void	OnOk();
	void	OnCancel();
	void	OnCbnSelChange();
	void	OnLvnItemChanged(NMLISTVIEW *pnmlv);
	void	OnMoveColumn(bool bUp);

	void	SaveCurrentColumnState(FolderType_t FolderType);
	void	SetupFolderColumns(FolderType_t FolderType);

	std::vector<Column_t>	&GetCurrentColumnList(FolderType_t FolderType);

	FolderColumns		&m_folderColumns;

	std::unordered_map<int,FolderType_t>	m_FolderMap;
	FolderType_t		m_PreviousFolderType;

	wil::unique_hicon	m_icon;

	CSetDefaultColumnsDialogPersistentSettings	*m_psdcdps;
};