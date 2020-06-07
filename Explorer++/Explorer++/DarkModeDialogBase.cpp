// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DarkModeDialogBase.h"
#include "DarkModeHelper.h"
#include "MainResource.h"

DarkModeDialogBase::DarkModeDialogBase(
	HINSTANCE hInstance, int iResource, HWND hParent, bool bResizable) :
	BaseDialog(hInstance, iResource, hParent, bResizable)
{
}

void DarkModeDialogBase::OnInitDialogBase()
{
	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return;
	}

	darkModeHelper.AllowDarkModeForWindow(m_hDlg, true);

	BOOL dark = TRUE;
	DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA compositionData = {
		DarkModeHelper::WCA_USEDARKMODECOLORS, &dark, sizeof(dark)
	};
	darkModeHelper.SetWindowCompositionAttribute(m_hDlg, &compositionData);

	AllowDarkModeForControls({ IDC_GRIPPER, IDOK, IDCANCEL });
}

void DarkModeDialogBase::AllowDarkModeForControls(const std::vector<int> controlIds)
{
	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return;
	}

	for (int controlId : controlIds)
	{
		if (HWND control = GetDlgItem(m_hDlg, controlId))
		{
			darkModeHelper.SetDarkModeForControl(control);
		}
	}
}

INT_PTR DarkModeDialogBase::OnCtlColorDlg(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(hdc);

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return FALSE;
	}

	return reinterpret_cast<INT_PTR>(darkModeHelper.GetBackgroundBrush());
}

INT_PTR DarkModeDialogBase::OnCtlColorStatic(HWND hwnd, HDC hdc)
{
	auto defaultRes = OnCtlColor(hwnd, hdc);

	auto res = OnCtlColorStaticExtra(hwnd, hdc);

	if (res)
	{
		return res;
	}

	return defaultRes;
}

INT_PTR DarkModeDialogBase::OnCtlColorEdit(HWND hwnd, HDC hdc)
{
	auto defaultRes = OnCtlColor(hwnd, hdc);

	auto res = OnCtlColorEditExtra(hwnd, hdc);

	if (res)
	{
		return res;
	}

	return defaultRes;
}

INT_PTR DarkModeDialogBase::OnCtlColorListBox(HWND hwnd, HDC hdc)
{
	auto defaultRes = OnCtlColor(hwnd, hdc);

	auto res = OnCtlColorListBoxExtra(hwnd, hdc);

	if (res)
	{
		return res;
	}

	return defaultRes;
}

INT_PTR DarkModeDialogBase::OnCtlColor(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return FALSE;
	}

	SetBkColor(hdc, DarkModeHelper::BACKGROUND_COLOR);
	SetTextColor(hdc, DarkModeHelper::FOREGROUND_COLOR);

	return reinterpret_cast<INT_PTR>(darkModeHelper.GetBackgroundBrush());
}

INT_PTR DarkModeDialogBase::OnCtlColorStaticExtra(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(hdc);

	return FALSE;
}

INT_PTR DarkModeDialogBase::OnCtlColorEditExtra(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(hdc);

	return FALSE;
}

INT_PTR DarkModeDialogBase::OnCtlColorListBoxExtra(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(hdc);

	return FALSE;
}