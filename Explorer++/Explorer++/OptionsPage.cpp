// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "OptionsPage.h"
#include "DarkModeButton.h"
#include "DarkModeGroupBox.h"
#include "DarkModeHelper.h"
#include "ResourceHelper.h"
#include "../Helper/ResizableDialogHelper.h"
#include <memory>
#include <unordered_set>

using namespace DarkModeButton;

OptionsPage::OptionsPage(UINT dialogResourceId, UINT titleResourceId, HWND parent,
	HINSTANCE resourceInstance, Config *config, CoreInterface *coreInterface,
	SettingChangedCallback settingChangedCallback, HWND tooltipWindow) :
	m_dialogResourceId(dialogResourceId),
	m_titleResourceId(titleResourceId),
	m_parent(parent),
	m_config(config),
	m_coreInterface(coreInterface),
	m_resourceInstance(resourceInstance),
	m_settingChangedCallback(settingChangedCallback),
	m_tooltipWindow(tooltipWindow)
{
}

OptionsPage::~OptionsPage() = default;

// Actual dialog creation is a separate method, since it's not possible to create the dialog in
// the constructor for this class. The reason being that the handler for WM_INITDIALOG will
// attempt to call virtual methods, which will fail if the derived class hasn't been fully
// instantiated yet.
void OptionsPage::InitializeDialog()
{
	if (m_dialog)
	{
		assert(false);
		return;
	}

	CreateDialogParam(m_resourceInstance, MAKEINTRESOURCE(m_dialogResourceId), m_parent,
		DialogProcStub, reinterpret_cast<LPARAM>(this));
}

HWND OptionsPage::GetDialog() const
{
	return m_dialog;
}

std::wstring OptionsPage::GetTitle() const
{
	return ResourceHelper::LoadString(m_resourceInstance, m_titleResourceId);
}

INT_PTR CALLBACK OptionsPage::DialogProcStub(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto *optionsDialogPage = reinterpret_cast<OptionsPage *>(GetWindowLongPtr(dlg, GWLP_USERDATA));

	switch (msg)
	{
	case WM_INITDIALOG:
	{
		optionsDialogPage = reinterpret_cast<OptionsPage *>(lParam);

		SetLastError(0);
		[[maybe_unused]] auto res =
			SetWindowLongPtr(dlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(optionsDialogPage));
		assert(!(res == 0 && GetLastError() != 0));
	}
	break;

	case WM_NCDESTROY:
		SetWindowLongPtr(dlg, GWLP_USERDATA, 0);
		return 0;
	}

	if (!optionsDialogPage)
	{
		return 0;
	}

	return optionsDialogPage->DialogProc(dlg, msg, wParam, lParam);
}

INT_PTR CALLBACK OptionsPage::DialogProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		m_dialog = dlg;

		m_resizableDialogHelper = InitializeResizeDialogHelper();
		InitializeControls();
	}
	break;

	case WM_CTLCOLORDLG:
		return OnPageCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
		return OnCtlColor(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_COMMAND:
		OnCommand(wParam, lParam);
		break;

	case WM_NOTIFY:
	{
		auto *nmhdr = reinterpret_cast<NMHDR *>(lParam);

		switch (nmhdr->code)
		{
		case NM_CUSTOMDRAW:
		{
			auto result = OnCustomDraw(reinterpret_cast<NMCUSTOMDRAW *>(lParam));
			SetWindowLongPtr(dlg, DWLP_MSGRESULT, result);
			return result;
		}
		break;
		}

		return OnNotify(wParam, lParam);
	}
	break;

	case WM_SIZE:
		m_resizableDialogHelper->UpdateControls(LOWORD(lParam), HIWORD(lParam));
		return 0;
	}

	return 0;
}

INT_PTR OptionsPage::OnPageCtlColorDlg(HWND hwnd, HDC hdc)
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

INT_PTR OptionsPage::OnCtlColor(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return FALSE;
	}

	SetBkColor(hdc, DarkModeHelper::BACKGROUND_COLOR);
	SetTextColor(hdc, DarkModeHelper::TEXT_COLOR);

	return reinterpret_cast<INT_PTR>(darkModeHelper.GetBackgroundBrush());
}

INT_PTR OptionsPage::OnCustomDraw(const NMCUSTOMDRAW *customDraw)
{
	bool isStoredCheckbox =
		(m_checkboxControlIds.count(static_cast<int>(customDraw->hdr.idFrom)) == 1);
	bool isStoredRadioButton =
		(m_radioButtonControlIds.count(static_cast<int>(customDraw->hdr.idFrom)) == 1);

	if (!isStoredCheckbox && !isStoredRadioButton)
	{
		return CDRF_DODEFAULT;
	}

	switch (customDraw->dwDrawStage)
	{
	case CDDS_PREPAINT:
		DrawButtonText(customDraw, isStoredCheckbox ? ButtonType::Checkbox : ButtonType::Radio);
		return CDRF_SKIPDEFAULT;
	}

	return CDRF_DODEFAULT;
}

void OptionsPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
}

INT_PTR OptionsPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	return 0;
}
