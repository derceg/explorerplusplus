// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialogHelper.h"
#include <boost/signals2.hpp>
#include <MsXml2.h>
#include <objbase.h>
#include <vector>

class FilterDialog;
class ShellBrowser;

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
	FilterDialogPersistentSettings &operator=(const FilterDialogPersistentSettings &);

	void SaveExtraRegistrySettings(HKEY hKey) override;
	void LoadExtraRegistrySettings(HKEY hKey) override;

	void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode) override;
	void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue) override;

	std::list<std::wstring> m_FilterList;
};

class FilterDialog : public BaseDialog
{
public:
	FilterDialog(const ResourceLoader *resourceLoader, HWND hParent, ShellBrowser *shellBrowser);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:
	std::vector<ResizableDialogControl> GetResizableControls() override;
	void SaveState() override;

	void OnShellBrowserDestroyed();

	void OnOk();
	void OnCancel();

	ShellBrowser *const m_shellBrowser;
	std::vector<boost::signals2::scoped_connection> m_connections;

	FilterDialogPersistentSettings *m_persistentSettings;
};
