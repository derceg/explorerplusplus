// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/FileOperations.h"
#include "../Helper/ResizableDialog.h"
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
	DestroyFilesDialogPersistentSettings &operator=(const DestroyFilesDialogPersistentSettings &);

	void SaveExtraRegistrySettings(HKEY hKey) override;
	void LoadExtraRegistrySettings(HKEY hKey) override;

	void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode) override;
	void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue) override;

	NFileOperations::OverwriteMethod_t m_uOverwriteMethod;
};

class DestroyFilesDialog : public BaseDialog
{
public:
	DestroyFilesDialog(HINSTANCE hInstance, HWND hParent,
		const std::list<std::wstring> &FullFilenameList, BOOL bShowFriendlyDates);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCtlColorStatic(HWND hwnd, HDC hdc) override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

private:
	void GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc,
		std::list<ResizableDialog::Control_t> &ControlList) override;
	void SaveState() override;

	void OnOk();
	void OnCancel();
	void OnConfirmDestroy();

	std::list<std::wstring> m_FullFilenameList;

	wil::unique_hicon m_icon;

	DestroyFilesDialogPersistentSettings *m_pdfdps;

	BOOL m_bShowFriendlyDates;
};