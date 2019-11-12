// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <vector>
#include "Explorer++_internal.h"
#include "ColorRuleHelper.h"
#include "DpiCompatibility.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/ResizableDialog.h"
#include "../Helper/DialogSettings.h"
#include <wil/resource.h>

class CCustomizeColorsDialog;

class CCustomizeColorsDialogPersistentSettings : public CDialogSettings
{
public:

	~CCustomizeColorsDialogPersistentSettings();

	static CCustomizeColorsDialogPersistentSettings &GetInstance();

private:

	friend CCustomizeColorsDialog;

	static const TCHAR SETTINGS_KEY[];

	CCustomizeColorsDialogPersistentSettings();

	CCustomizeColorsDialogPersistentSettings(const CCustomizeColorsDialogPersistentSettings &);
	CCustomizeColorsDialogPersistentSettings & operator=(const CCustomizeColorsDialogPersistentSettings &);
};

class CCustomizeColorsDialog : public CBaseDialog
{
public:

	CCustomizeColorsDialog(HINSTANCE hInstance,int iResource,HWND hParent,std::vector<NColorRuleHelper::ColorRule_t> *pColorRuleList);
	~CCustomizeColorsDialog();

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnNotify(NMHDR *pnmhdr);
	INT_PTR	OnClose();

private:

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);
	void	SaveState();

	void	OnNew();
	void	OnEdit();
	void	InsertColorRuleIntoListView(HWND hListView,const NColorRuleHelper::ColorRule_t &ColorRule,int iIndex);
	void	EditColorRule(int iSelected);
	void	OnMove(BOOL bUp);
	void	OnDelete();

	void	OnOk();
	void	OnCancel();

	DpiCompatibility m_dpiCompat;
	wil::unique_hicon m_icon;

	std::vector<NColorRuleHelper::ColorRule_t> *m_pColorRuleList;

	CCustomizeColorsDialogPersistentSettings *m_pccdps;
};