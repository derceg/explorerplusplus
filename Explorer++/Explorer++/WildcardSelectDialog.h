// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "SelectionType.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialogHelper.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <MsXml2.h>
#include <objbase.h>
#include <list>
#include <string>
#include <vector>

class ShellBrowser;
class WildcardSelectDialog;

class WildcardSelectDialogPersistentSettings : public DialogSettings
{
public:
	static WildcardSelectDialogPersistentSettings &GetInstance();

private:
	friend WildcardSelectDialog;

	static const TCHAR SETTINGS_KEY[];

	static const TCHAR SETTING_PATTERN_LIST[];
	static const TCHAR SETTING_CURRENT_TEXT[];

	WildcardSelectDialogPersistentSettings();

	WildcardSelectDialogPersistentSettings(const WildcardSelectDialogPersistentSettings &) = delete;
	WildcardSelectDialogPersistentSettings &operator=(
		const WildcardSelectDialogPersistentSettings &) = delete;

	void SaveExtraRegistrySettings(HKEY hKey) override;
	void LoadExtraRegistrySettings(HKEY hKey) override;

	void SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode) override;
	void LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue) override;

	std::wstring m_pattern;
	std::list<std::wstring> m_PatternList;
};

class WildcardSelectDialog : public BaseDialog
{
public:
	WildcardSelectDialog(const ResourceLoader *resourceLoader, HWND parent,
		ShellBrowser *shellBrowser, SelectionType selectionType);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

private:
	std::vector<ResizableDialogControl> GetResizableControls() override;
	void SaveState() override;

	void OnShellBrowserDestroyed();

	void OnOk();
	void OnCancel();

	ShellBrowser *const m_shellBrowser;
	const SelectionType m_selectionType;
	wil::unique_hicon m_icon;
	std::vector<boost::signals2::scoped_connection> m_connections;

	WildcardSelectDialogPersistentSettings *m_pwsdps = nullptr;
};
