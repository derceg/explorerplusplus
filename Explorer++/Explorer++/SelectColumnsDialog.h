// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "TabContainer.h"
#include "TabInterface.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"

class CSelectColumnsDialog;

class CSelectColumnsDialogPersistentSettings : public CDialogSettings
{
public:

	~CSelectColumnsDialogPersistentSettings();

	static CSelectColumnsDialogPersistentSettings &GetInstance();

private:

	friend CSelectColumnsDialog;

	static const TCHAR SETTINGS_KEY[];

	CSelectColumnsDialogPersistentSettings();

	CSelectColumnsDialogPersistentSettings(const CSelectColumnsDialogPersistentSettings &);
	CSelectColumnsDialogPersistentSettings & operator=(const CSelectColumnsDialogPersistentSettings &);
};

class CSelectColumnsDialog : public CBaseDialog
{
public:

	CSelectColumnsDialog(HINSTANCE hInstance, int iResource, HWND hParent, IExplorerplusplus *pexpp,
		TabContainer *tabContainer, TabInterface *ti);
	~CSelectColumnsDialog();

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnNotify(NMHDR *pnmhdr);
	INT_PTR	OnClose();

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	bool	CompareColumns(const Column_t &column1, const Column_t &column2);

	void	GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);
	void	SaveState();

	void	OnOk();
	void	OnCancel();
	void	OnLvnItemChanged(NMLISTVIEW *pnmlv);
	void	OnMoveColumn(bool bUp);

	IExplorerplusplus *m_pexpp;
	TabContainer *m_tabContainer;
	TabInterface *m_ti;
	BOOL m_bColumnsSwapped;

	CSelectColumnsDialogPersistentSettings *m_pscdps;
};