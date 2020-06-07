// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CustomizeColorsDialog.h"
#include "DarkModeDialogBase.h"
#include "../Helper/DialogSettings.h"

class ColorRuleDialog;

class ColorRuleDialogPersistentSettings : public DialogSettings
{
public:

	static ColorRuleDialogPersistentSettings &GetInstance();

private:

	friend ColorRuleDialog;

	static const TCHAR SETTINGS_KEY[];
	static const COLORREF DEFAULT_INITIAL_COLOR;

	static const TCHAR SETTING_INITIAL_COLOR[];
	static const TCHAR SETTING_CUSTOM_COLORS[];

	ColorRuleDialogPersistentSettings();

	ColorRuleDialogPersistentSettings(const ColorRuleDialogPersistentSettings &);
	ColorRuleDialogPersistentSettings & operator=(const ColorRuleDialogPersistentSettings &);

	void		SaveExtraRegistrySettings(HKEY hKey) override;
	void		LoadExtraRegistrySettings(HKEY hKey) override;

	void		SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode) override;
	void		LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue) override;

	COLORREF	m_cfInitialColor;
	COLORREF	m_cfCustomColors[16];
};

class ColorRuleDialog : public DarkModeDialogBase
{
public:

	ColorRuleDialog(HINSTANCE hInstance, HWND hParent, NColorRuleHelper::ColorRule *pColorRule, BOOL bEdit);

	LRESULT CALLBACK	StaticColorProc(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

protected:

	INT_PTR	OnInitDialog() override;
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam) override;
	INT_PTR	OnClose() override;

	void	SaveState() override;

private:

	void	OnChangeColor();

	void	OnOk();
	void	OnCancel();

	BOOL		m_bEdit;
	NColorRuleHelper::ColorRule	*m_pColorRule;

	COLORREF	m_cfCurrentColor;

	ColorRuleDialogPersistentSettings	*m_pcrdps;
};