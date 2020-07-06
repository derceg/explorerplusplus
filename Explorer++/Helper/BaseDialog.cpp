// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BaseDialog.h"
#include "Controls.h"
#include "CustomGripper.h"
#include "DpiCompatibility.h"
#include "Helper.h"
#include "WindowHelper.h"
#include <unordered_map>

namespace
{
	std::unordered_map<HWND, BaseDialog *> g_windowMap;
}

BaseDialog::BaseDialog(HINSTANCE hInstance, int iResource, HWND hParent, bool bResizable) :
	m_hInstance(hInstance),
	m_iResource(iResource),
	m_hParent(hParent),
	m_bResizable(bResizable)
{
	m_prd = nullptr;
	m_bShowingModelessDialog = FALSE;
}

INT_PTR CALLBACK BaseDialogProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		/* Store a mapping between window handles
		and objects. This must be done, as each
		dialog is managed by a separate object,
		but all window calls come through this
		function.
		Since two or more dialogs may be
		shown at once (as a dialog can be
		modeless), this function needs to be able
		to send the specified messages to the
		correct object.
		May also use thunks - see
		http://www.hackcraft.net/cpp/windowsThunk/ */
		g_windowMap.insert(std::unordered_map<HWND, BaseDialog *>::value_type(
			hDlg, reinterpret_cast<BaseDialog *>(lParam)));
	}
	break;
	}

	auto itr = g_windowMap.find(hDlg);

	if (itr != g_windowMap.end())
	{
		return itr->second->BaseDialogProc(hDlg, uMsg, wParam, lParam);
	}

	return 0;
}

INT_PTR CALLBACK BaseDialog::BaseDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		m_hDlg = hDlg;

		if (m_bResizable)
		{
			RECT windowRect;
			GetWindowRect(m_hDlg, &windowRect);

			/* Assume that the current width and height of
			the dialog are the minimum width and height.
			Note that at this point, the dialog has NOT
			been initialized in any way, so it will not
			have had a chance to be resized yet. */
			m_iMinWidth = GetRectWidth(&windowRect);
			m_iMinHeight = GetRectHeight(&windowRect);

			RECT clientRect;
			GetClientRect(m_hDlg, &clientRect);

			const SIZE gripperSize = CustomGripper::GetDpiScaledSize(m_hDlg);

			CreateWindow(CustomGripper::CLASS_NAME, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
				clientRect.right - gripperSize.cx, clientRect.bottom - gripperSize.cy,
				gripperSize.cx, gripperSize.cy, m_hDlg,
				reinterpret_cast<HMENU>(static_cast<INT_PTR>(GetGripperControlId())),
				GetModuleHandle(nullptr), nullptr);

			m_dsc = DialogSizeConstraint::None;

			std::list<ResizableDialog::Control> controlList;

			ResizableDialog::Control control;
			control.iID = GetGripperControlId();
			control.Type = ResizableDialog::ControlType::Move;
			control.Constraint = ResizableDialog::ControlConstraint::None;
			controlList.push_back(control);

			GetResizableControlInformation(m_dsc, controlList);

			m_prd = std::make_unique<ResizableDialog>(m_hDlg, controlList);
		}

		auto &dpiCompat = DpiCompatibility::GetInstance();
		UINT dpi = dpiCompat.GetDpiForWindow(m_hDlg);
		int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
		int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
		m_icon = GetDialogIcon(iconWidth, iconHeight);

		if (m_icon)
		{
			SetClassLongPtr(m_hDlg, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(m_icon.get()));
		}

		m_tipWnd = CreateTooltipControl(m_hDlg, m_hInstance);

		OnInitDialogBase();
	}
	break;

	case WM_GETMINMAXINFO:
		if (m_bResizable)
		{
			auto pmmi = reinterpret_cast<LPMINMAXINFO>(lParam);

			pmmi->ptMinTrackSize.x = m_iMinWidth;
			pmmi->ptMinTrackSize.y = m_iMinHeight;

			if (m_dsc == DialogSizeConstraint::X)
			{
				pmmi->ptMaxTrackSize.y = m_iMinHeight;
			}

			if (m_dsc == DialogSizeConstraint::Y)
			{
				pmmi->ptMaxTrackSize.x = m_iMinWidth;
			}

			return 0;
		}
		break;

	case WM_SIZE:
		if (m_bResizable)
		{
			m_prd->UpdateControls(LOWORD(lParam), HIWORD(lParam));
			return 0;
		}
		break;

	case WM_DESTROY:
	{
		/* If this is a modeless dialog, notify the
		caller that the dialog is been destroyed. */
		if (m_bShowingModelessDialog)
		{
			if (m_pmdn != nullptr)
			{
				m_pmdn->OnModelessDialogDestroy(m_iResource);
				m_pmdn->Release();
			}
		}

		/* Within WM_DESTROY, all child windows
		still exist. */
		SaveState();
	}
	break;

	case WM_NCDESTROY:
		g_windowMap.erase(g_windowMap.find(hDlg));
		break;
	}

	return ForwardMessage(hDlg, uMsg, wParam, lParam);
}

wil::unique_hicon BaseDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	UNREFERENCED_PARAMETER(iconWidth);
	UNREFERENCED_PARAMETER(iconHeight);

	return nullptr;
}

void BaseDialog::OnInitDialogBase()
{
}

INT_PTR BaseDialog::GetDefaultReturnValue(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	return 0;
}

HINSTANCE BaseDialog::GetInstance() const
{
	return m_hInstance;
}

INT_PTR BaseDialog::ShowModalDialog()
{
	/* Explicitly disallow the creation of another
	dialog from this object while a modeless dialog
	is been shown. */
	if (m_bShowingModelessDialog)
	{
		return -1;
	}

	return DialogBoxParam(m_hInstance, MAKEINTRESOURCE(m_iResource), m_hParent, BaseDialogProcStub,
		reinterpret_cast<LPARAM>(this));
}

HWND BaseDialog::ShowModelessDialog(IModelessDialogNotification *pmdn)
{
	if (m_bShowingModelessDialog)
	{
		return nullptr;
	}

	HWND hDlg = CreateDialogParam(m_hInstance, MAKEINTRESOURCE(m_iResource), m_hParent,
		BaseDialogProcStub, reinterpret_cast<LPARAM>(this));

	if (hDlg != nullptr)
	{
		m_bShowingModelessDialog = TRUE;
	}

	m_pmdn = pmdn;

	return hDlg;
}

void BaseDialog::GetResizableControlInformation(
	DialogSizeConstraint &dsc, std::list<ResizableDialog::Control> &controlList)
{
	UNREFERENCED_PARAMETER(dsc);
	UNREFERENCED_PARAMETER(controlList);
}

void BaseDialog::SaveState()
{
}

void BaseDialog::AddTooltipForControl(int controlId, int stringResourceId)
{
	TOOLINFO toolInfo = {};
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.hwnd = m_hDlg;
	toolInfo.uId = reinterpret_cast<UINT_PTR>(GetDlgItem(m_hDlg, controlId));
	toolInfo.hinst = m_hInstance;
	toolInfo.lpszText = MAKEINTRESOURCE(stringResourceId);
	SendMessage(m_tipWnd, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));
}