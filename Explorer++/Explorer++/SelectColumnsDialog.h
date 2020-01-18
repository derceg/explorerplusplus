// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "TabContainer.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"

class SelectColumnsDialog;

class SelectColumnsDialogPersistentSettings : public DialogSettings
{
public:

	static SelectColumnsDialogPersistentSettings &GetInstance();

private:

	friend SelectColumnsDialog;

	static const TCHAR SETTINGS_KEY[];

	SelectColumnsDialogPersistentSettings();

	SelectColumnsDialogPersistentSettings(const SelectColumnsDialogPersistentSettings &);
	SelectColumnsDialogPersistentSettings & operator=(const SelectColumnsDialogPersistentSettings &);
};

class SelectColumnsDialog : public BaseDialog
{
public:

	SelectColumnsDialog(HINSTANCE hInstance, HWND hParent, IExplorerplusplus *pexpp,
		TabContainer *tabContainer);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnNotify(NMHDR *pnmhdr);
	INT_PTR	OnClose();

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:

	bool	CompareColumns(const Column_t &column1, const Column_t &column2);

	void	GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc, std::list<ResizableDialog::Control_t> &ControlList);
	void	SaveState();

	void	OnOk();
	void	OnCancel();
	BOOL	OnLvnItemChanging(const NMLISTVIEW *nmlv);
	void	OnLvnItemChanged(const NMLISTVIEW *pnmlv);
	void	OnMoveColumn(bool bUp);

	IExplorerplusplus *m_pexpp;
	TabContainer *m_tabContainer;
	BOOL m_bColumnsSwapped;

	SelectColumnsDialogPersistentSettings *m_pscdps;
};