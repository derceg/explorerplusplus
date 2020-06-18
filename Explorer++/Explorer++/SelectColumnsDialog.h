// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DarkModeDialogBase.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"

struct Column_t;
class IconResourceLoader;
class SelectColumnsDialog;
class ShellBrowser;

class SelectColumnsDialogPersistentSettings : public DialogSettings
{
public:
	static SelectColumnsDialogPersistentSettings &GetInstance();

private:
	friend SelectColumnsDialog;

	static const TCHAR SETTINGS_KEY[];

	SelectColumnsDialogPersistentSettings();

	SelectColumnsDialogPersistentSettings(const SelectColumnsDialogPersistentSettings &);
	SelectColumnsDialogPersistentSettings &operator=(const SelectColumnsDialogPersistentSettings &);
};

class SelectColumnsDialog : public DarkModeDialogBase
{
public:
	SelectColumnsDialog(HINSTANCE hInstance, HWND hParent, ShellBrowser *shellBrowser,
		IconResourceLoader *iconResourceLoader);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *pnmhdr) override;
	INT_PTR OnClose() override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:
	bool CompareColumns(const Column_t &column1, const Column_t &column2);

	void GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc,
		std::list<ResizableDialog::Control> &ControlList) override;
	void SaveState() override;

	void OnOk();
	void OnCancel();
	BOOL OnLvnItemChanging(const NMLISTVIEW *nmlv);
	void OnLvnItemChanged(const NMLISTVIEW *pnmlv);
	void OnMoveColumn(bool bUp);

	ShellBrowser *m_shellBrowser;
	IconResourceLoader *m_iconResourceLoader;
	BOOL m_bColumnsSwapped;

	SelectColumnsDialogPersistentSettings *m_persistentSettings;
};