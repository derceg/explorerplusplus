// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"
#include <MsXml2.h>
#include <objbase.h>

class FilterDialog;

class FilterDialogPersistentSettings : public DialogSettings
{
public:

	static FilterDialogPersistentSettings &GetInstance();

private:

	friend FilterDialog;

	static const TCHAR SETTINGS_KEY[];

	static const TCHAR SETTING_FILTER_LIST[];

	FilterDialogPersistentSettings();

	FilterDialogPersistentSettings(const FilterDialogPersistentSettings &);
	FilterDialogPersistentSettings & operator=(const FilterDialogPersistentSettings &);

	void SaveExtraRegistrySettings(HKEY hKey);
	void LoadExtraRegistrySettings(HKEY hKey);

	void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode);
	void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue);

	std::list<std::wstring>	m_FilterList;
};

class FilterDialog : public BaseDialog
{
public:

	FilterDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *pexpp);

protected:

	INT_PTR				OnInitDialog();
	INT_PTR				OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR				OnClose();

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	void				GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc, std::list<ResizableDialog::Control_t> &ControlList);
	void				SaveState();

	void				OnOk();
	void				OnCancel();

	IExplorerplusplus *m_pexpp;

	FilterDialogPersistentSettings *m_pfdps;
};