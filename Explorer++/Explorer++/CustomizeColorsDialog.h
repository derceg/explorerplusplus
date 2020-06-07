// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ColorRuleHelper.h"
#include "DarkModeDialogBase.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"
#include <vector>

class CustomizeColorsDialog;
__interface IExplorerplusplus;

class CustomizeColorsDialogPersistentSettings : public DialogSettings
{
public:

	static CustomizeColorsDialogPersistentSettings &GetInstance();

private:

	friend CustomizeColorsDialog;

	static const TCHAR SETTINGS_KEY[];

	CustomizeColorsDialogPersistentSettings();

	CustomizeColorsDialogPersistentSettings(const CustomizeColorsDialogPersistentSettings &);
	CustomizeColorsDialogPersistentSettings & operator=(const CustomizeColorsDialogPersistentSettings &);
};

class CustomizeColorsDialog : public DarkModeDialogBase
{
public:

	CustomizeColorsDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *expp,
		std::vector<NColorRuleHelper::ColorRule> *pColorRuleList);

protected:

	INT_PTR	OnInitDialog() override;
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam) override;
	INT_PTR	OnNotify(NMHDR *pnmhdr) override;
	INT_PTR	OnClose() override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	void	GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc, std::list<ResizableDialog::Control_t> &ControlList) override;
	void	SaveState() override;

	void	OnNew();
	void	OnEdit();
	void	InsertColorRuleIntoListView(HWND hListView,const NColorRuleHelper::ColorRule &colorRule,int iIndex);
	void	EditColorRule(int iSelected);
	void	OnMove(BOOL bUp);
	void	OnDelete();

	void	OnOk();
	void	OnCancel();

	IExplorerplusplus *m_expp;

	std::vector<NColorRuleHelper::ColorRule> *m_pColorRuleList;

	CustomizeColorsDialogPersistentSettings *m_persistentSettings;
};