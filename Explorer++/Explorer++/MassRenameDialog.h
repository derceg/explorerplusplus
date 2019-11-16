// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/ResizableDialog.h"

class CMassRenameDialog;

class CMassRenameDialogPersistentSettings : public CDialogSettings
{
public:

	static CMassRenameDialogPersistentSettings &GetInstance();

private:

	friend CMassRenameDialog;

	static const TCHAR SETTINGS_KEY[];

	static const TCHAR SETTING_COLUMN_WIDTH_1[];
	static const TCHAR SETTING_COLUMN_WIDTH_2[];

	static const int DEFAULT_MASS_RENAME_COLUMN_WIDTH = 250;

	CMassRenameDialogPersistentSettings();

	CMassRenameDialogPersistentSettings(const CMassRenameDialogPersistentSettings &);
	CMassRenameDialogPersistentSettings & operator=(const CMassRenameDialogPersistentSettings &);

	void SaveExtraRegistrySettings(HKEY hKey);
	void LoadExtraRegistrySettings(HKEY hKey);

	void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode);
	void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue);

	int	m_iColumnWidth1;
	int	m_iColumnWidth2;
};

class CMassRenameDialog : public CBaseDialog
{
public:

	CMassRenameDialog(HINSTANCE hInstance, int iResource, HWND hParent, IExplorerplusplus *expp,
		std::list<std::wstring> FullFilenameList, CFileActionHandler *pFileActionHandler);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);
	void	SaveState();

	void	OnOk();
	void	OnCancel();

	void	ProcessFileName(const std::wstring &strTarget,const std::wstring &strFilename,int iFileIndex,std::wstring &strOutput);

	IExplorerplusplus *m_expp;
	std::list<std::wstring> m_FullFilenameList;
	wil::unique_hicon m_moreIcon;
	CFileActionHandler *m_pFileActionHandler;

	CMassRenameDialogPersistentSettings *m_pmrdps;
};