// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ThemedDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/FileOperations.h"
#include "../Helper/ResizableDialogHelper.h"
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

	FileOperations::OverwriteMethod m_overwriteMethod;
};

class DestroyFilesDialog : public ThemedDialog
{
public:
	DestroyFilesDialog(const ResourceLoader *resourceLoader, HINSTANCE resourceInstance,
		HWND hParent, ThemeManager *themeManager, const std::list<std::wstring> &FullFilenameList,
		BOOL bShowFriendlyDates);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

private:
	std::vector<ResizableDialogControl> GetResizableControls() override;
	void SaveState() override;

	void OnOk();
	void OnCancel();
	void OnConfirmDestroy();

	std::list<std::wstring> m_FullFilenameList;

	wil::unique_hicon m_icon;

	DestroyFilesDialogPersistentSettings *m_pdfdps;

	BOOL m_bShowFriendlyDates;
};
