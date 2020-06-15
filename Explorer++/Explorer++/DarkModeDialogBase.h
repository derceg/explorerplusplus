// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <memory>
#include <unordered_set>
#include <vector>

class DarkModeDialogBase : public BaseDialog
{
protected:
	DarkModeDialogBase(HINSTANCE hInstance, int iResource, HWND hParent, bool bResizable);

	void AllowDarkModeForControls(const std::vector<int> controlIds);
	void AllowDarkModeForListView(int controlId);
	void AllowDarkModeForCheckboxes(const std::vector<int> controlIds);
	void AllowDarkModeForRadioButtons(const std::vector<int> controlIds);

	virtual INT_PTR OnCtlColorStaticExtra(HWND hwnd, HDC hdc);
	virtual INT_PTR OnCtlColorEditExtra(HWND hwnd, HDC hdc);
	virtual INT_PTR OnCtlColorListBoxExtra(HWND hwnd, HDC hdc);

private:
	// Also applies to radio buttons.
	static inline constexpr int CHECKBOX_TEXT_SPACING_96DPI = 3;

	enum class ButtonType
	{
		Checkbox,
		Radio
	};

	void OnInitDialogBase() override final;
	int GetGripperControlId() override final;

	LRESULT CALLBACK DialogWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCustomDraw(const NMCUSTOMDRAW *customDraw);

	// Handles checkboxes and radio buttons.
	void OnDrawButtonText(const NMCUSTOMDRAW *customDraw, ButtonType buttonType);

	LRESULT CALLBACK ListViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	INT_PTR OnCtlColorDlg(HWND hwnd, HDC hdc) override final;
	INT_PTR OnCtlColorStatic(HWND hwnd, HDC hdc) override final;
	INT_PTR OnCtlColorEdit(HWND hwnd, HDC hdc) override final;
	INT_PTR OnCtlColorListBox(HWND hwnd, HDC hdc) override final;
	INT_PTR OnCtlColor(HWND hwnd, HDC hdc);

	std::unordered_set<int> m_checkboxControlIds;
	std::unordered_set<int> m_radioButtonControlIds;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
};