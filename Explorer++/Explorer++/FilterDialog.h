// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "DpiCompatibility.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"
#include <wil/resource.h>
#include <MsXml2.h>
#include <objbase.h>

class CFilterDialog;

class CFilterDialogPersistentSettings : public CDialogSettings
{
public:

	~CFilterDialogPersistentSettings();

	static CFilterDialogPersistentSettings &GetInstance();

private:

	friend CFilterDialog;

	static const TCHAR SETTINGS_KEY[];

	static const TCHAR SETTING_FILTER_LIST[];

	CFilterDialogPersistentSettings();

	CFilterDialogPersistentSettings(const CFilterDialogPersistentSettings &);
	CFilterDialogPersistentSettings & operator=(const CFilterDialogPersistentSettings &);

	void SaveExtraRegistrySettings(HKEY hKey);
	void LoadExtraRegistrySettings(HKEY hKey);

	void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode);
	void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue);

	std::list<std::wstring>	m_FilterList;
};

class CFilterDialog : public CBaseDialog
{
public:

	CFilterDialog(HINSTANCE hInstance,int iResource,HWND hParent,IExplorerplusplus *pexpp);
	~CFilterDialog();

protected:

	INT_PTR				OnInitDialog();
	INT_PTR				OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR				OnClose();

private:

	void				GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);
	void				SaveState();

	void				OnOk();
	void				OnCancel();

	IExplorerplusplus *m_pexpp;
	DpiCompatibility m_dpiCompat;
	wil::unique_hicon m_icon;

	CFilterDialogPersistentSettings *m_pfdps;
};