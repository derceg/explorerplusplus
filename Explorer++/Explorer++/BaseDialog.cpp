// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BaseDialog.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ImageInterop.h"
#include "../Helper/WindowHelper.h"
#include <glog/logging.h>

BaseDialog::BaseDialog(const ResourceLoader *resourceLoader, int iResource, HWND hParent,
	DialogSizingType dialogSizingType, std::optional<Icon> iconType) :
	m_resourceLoader(resourceLoader),
	m_iResource(iResource),
	m_hParent(hParent),
	m_dialogSizingType(dialogSizingType),
	m_iconType(iconType)
{
}

INT_PTR CALLBACK BaseDialog::BaseDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		m_hDlg = hDlg;

		m_tipWnd = CreateTooltipControl(m_hDlg);

		AddDynamicControls();

		if (m_dialogSizingType != DialogSizingType::None)
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

			// The presence of the SBS_SIZEBOXBOTTOMRIGHTALIGN style will cause the control to align
			// itself with the bottom right corner of the rectangle specified by (x, y, width,
			// height). Additionally, the size of the control will be set to the default size for
			// system size boxes.
			HWND gripper = CreateWindow(WC_SCROLLBAR, L"",
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBS_SIZEGRIP
					| SBS_SIZEBOXBOTTOMRIGHTALIGN,
				0, 0, clientRect.right, clientRect.bottom, m_hDlg, 0, GetModuleHandle(nullptr),
				nullptr);

			std::vector<ResizableDialogControl> controls = GetResizableControls();
			controls.emplace_back(gripper, MovingType::Both, SizingType::None);
			m_resizableDialogHelper = std::make_unique<ResizableDialogHelper>(m_hDlg, controls);
		}

		SetDialogIcon();
	}
	break;

	case WM_GETMINMAXINFO:
		if (m_dialogSizingType != DialogSizingType::None)
		{
			auto pmmi = reinterpret_cast<LPMINMAXINFO>(lParam);

			pmmi->ptMinTrackSize.x = m_iMinWidth;
			pmmi->ptMinTrackSize.y = m_iMinHeight;

			if (m_dialogSizingType == DialogSizingType::Horizontal)
			{
				pmmi->ptMaxTrackSize.y = m_iMinHeight;
			}

			if (m_dialogSizingType == DialogSizingType::Vertical)
			{
				pmmi->ptMaxTrackSize.x = m_iMinWidth;
			}

			return 0;
		}
		break;

	case WM_SIZE:
		if (m_dialogSizingType != DialogSizingType::None)
		{
			m_resizableDialogHelper->UpdateControls(LOWORD(lParam), HIWORD(lParam));
			return 0;
		}
		break;

	case WM_DESTROY:
	{
		if (m_showingModelessDialog && m_modelessDialogDestroyedObserver)
		{
			m_modelessDialogDestroyedObserver();
		}

		/* Within WM_DESTROY, all child windows
		still exist. */
		SaveState();
	}
	break;
	}

	return ForwardMessage(hDlg, uMsg, wParam, lParam);
}

void BaseDialog::AddDynamicControls()
{
}

void BaseDialog::SetDialogIcon()
{
	wil::unique_hicon icon;

	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(m_hDlg);
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);

	if (m_iconType)
	{
		auto bitmap = m_resourceLoader->LoadBitmap(*m_iconType, iconWidth, iconHeight);

		HRESULT hr = ImageInterop::CreateHICONFromWICBitmap(bitmap.get(), icon);
		CHECK(SUCCEEDED(hr));
	}
	else
	{
		icon.reset(reinterpret_cast<HICON>(LoadImage(GetModuleHandle(nullptr),
			MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, iconWidth, iconHeight, LR_DEFAULTCOLOR)));
		CHECK(icon);
	}

	SendMessage(m_hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon.get()));
	m_icon = std::move(icon);
}

INT_PTR BaseDialog::GetDefaultReturnValue(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	return 0;
}

INT_PTR BaseDialog::ShowModalDialog()
{
	/* Explicitly disallow the creation of another
	dialog from this object while a modeless dialog
	is been shown. */
	if (m_showingModelessDialog)
	{
		return -1;
	}

	return m_resourceLoader->CreateModalDialog(m_iResource, m_hParent,
		std::bind_front(&BaseDialog::BaseDialogProc, this));
}

HWND BaseDialog::ShowModelessDialog(std::function<void()> dialogDestroyedObserver)
{
	if (m_showingModelessDialog)
	{
		return nullptr;
	}

	m_showingModelessDialog = true;
	m_modelessDialogDestroyedObserver = dialogDestroyedObserver;

	return m_resourceLoader->CreateModelessDialog(m_iResource, m_hParent,
		std::bind_front(&BaseDialog::BaseDialogProc, this));
}

std::vector<ResizableDialogControl> BaseDialog::GetResizableControls()
{
	return {};
}

void BaseDialog::SaveState()
{
}

INT_PTR BaseDialog::OnNcDestroy()
{
	delete this;
	return 0;
}
