// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ThirdPartyCreditsDialog.h"
#include "DarkModeHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/RichEditHelper.h"
#include "../Helper/WindowHelper.h"

ThirdPartyCreditsDialog::ThirdPartyCreditsDialog(HINSTANCE instance, HWND parent) :
	DarkModeDialogBase(instance, IDD_THIRD_PARTY_CREDITS, parent, false)
{
}

INT_PTR ThirdPartyCreditsDialog::OnInitDialog()
{
	SendDlgItemMessage(m_hDlg, IDC_CREDITS, EM_AUTOURLDETECT, AURL_ENABLEURL, NULL);
	SendDlgItemMessage(m_hDlg, IDC_CREDITS, EM_SETEVENTMASK, 0, ENM_LINK);

	std::wstring credits = ResourceHelper::LoadString(GetInstance(), IDS_THIRD_PARTY_CREDITS);
	SetDlgItemText(m_hDlg, IDC_CREDITS, credits.c_str());

	if (DarkModeHelper::GetInstance().IsDarkModeEnabled())
	{
		SendDlgItemMessage(
			m_hDlg, IDC_CREDITS, EM_SETBKGNDCOLOR, 0, DarkModeHelper::BACKGROUND_COLOR);

		CHARFORMAT charFormat;
		charFormat.cbSize = sizeof(charFormat);
		charFormat.dwMask = CFM_COLOR;
		charFormat.crTextColor = DarkModeHelper::TEXT_COLOR;
		charFormat.dwEffects = 0;
		SendDlgItemMessage(
			m_hDlg, IDC_CREDITS, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&charFormat));
	}

	CenterWindow(GetParent(m_hDlg), m_hDlg);

	return TRUE;
}

INT_PTR ThirdPartyCreditsDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDCANCEL:
		EndDialog(m_hDlg, 0);
		break;
	}

	return 0;
}

INT_PTR ThirdPartyCreditsDialog::OnNotify(NMHDR *pnmhdr)
{
	switch (pnmhdr->code)
	{
	case EN_LINK:
		return OnLinkNotification(reinterpret_cast<ENLINK *>(pnmhdr));
	}

	return 0;
}

INT_PTR ThirdPartyCreditsDialog::OnLinkNotification(const ENLINK *linkNotificationDetails)
{
	if (linkNotificationDetails->nmhdr.idFrom != IDC_CREDITS)
	{
		return 0;
	}

	switch (linkNotificationDetails->msg)
	{
	case WM_LBUTTONUP:
		OnLinkClicked(linkNotificationDetails);
		return 1;
	}

	return 0;
}

void ThirdPartyCreditsDialog::OnLinkClicked(const ENLINK *linkNotificationDetails)
{
	std::wstring text = GetRichEditLinkText(linkNotificationDetails);
	ShellExecute(nullptr, L"open", text.c_str(), nullptr, nullptr, SW_SHOW);
}

INT_PTR ThirdPartyCreditsDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}