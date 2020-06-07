// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"

class DarkModeDialogBase : public BaseDialog
{
protected:
	DarkModeDialogBase(HINSTANCE hInstance, int iResource, HWND hParent, bool bResizable);

	void AllowDarkModeForControls(const std::vector<int> controlIds);

	virtual INT_PTR OnCtlColorStaticExtra(HWND hwnd, HDC hdc);
	virtual INT_PTR OnCtlColorEditExtra(HWND hwnd, HDC hdc);
	virtual INT_PTR OnCtlColorListBoxExtra(HWND hwnd, HDC hdc);

private:
	void OnInitDialogBase() override final;

	INT_PTR OnCtlColorDlg(HWND hwnd, HDC hdc) override final;
	INT_PTR OnCtlColorStatic(HWND hwnd, HDC hdc) override final;
	INT_PTR OnCtlColorEdit(HWND hwnd, HDC hdc) override final;
	INT_PTR OnCtlColorListBox(HWND hwnd, HDC hdc) override final;
	INT_PTR OnCtlColor(HWND hwnd, HDC hdc);
};