// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/FileOperations.h"
#include <wil/resource.h>

class DestroyFilesDialog;

class DestroyFilesDialogPersistentSettings : public DialogSettings
{
public:

	static DestroyFilesDialogPersistentSettings &GetInstance();

private:

	friend DestroyFilesDialog;

	static const TCHAR SETTINGS_KEY[];

	static const TCHAR SETTING_OVERWRITE_METHOD[];

	DestroyFilesDialogPersistentSettings();

	DestroyFilesDialogPersistentSettings(const DestroyFilesDialogPersistentSettings &);
	DestroyFilesDialogPersistentSettings & operator=(const DestroyFilesDialogPersistentSettings &);

	void SaveExtraRegistrySettings(HKEY hKey);
	void LoadExtraRegistrySettings(HKEY hKey);

	void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode);
	void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue);

	NFileOperations::OverwriteMethod_t	m_uOverwriteMethod;
};

class DestroyFilesDialog : public BaseDialog
{
public:

	DestroyFilesDialog(HINSTANCE hInstance, HWND hParent, std::list<std::wstring> FullFilenameList, BOOL bShowFriendlyDates);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCtlColorStatic(HWND hwnd,HDC hdc);
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();

private:

	void	GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc, std::list<ResizableDialog::Control_t> &ControlList);
	void	SaveState();

	void	OnOk();
	void	OnCancel();
	void	OnConfirmDestroy();

	std::list<std::wstring>	m_FullFilenameList;

	wil::unique_hicon	m_icon;

	DestroyFilesDialogPersistentSettings	*m_pdfdps;

	BOOL	m_bShowFriendlyDates;
};