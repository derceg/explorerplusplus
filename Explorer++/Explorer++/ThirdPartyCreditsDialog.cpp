// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ThirdPartyCreditsDialog.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/WindowHelper.h"

ThirdPartyCreditsDialog::ThirdPartyCreditsDialog(HINSTANCE instance, HWND parent) :
	BaseDialog(instance, IDD_THIRD_PARTY_CREDITS, parent, false)
{
}

INT_PTR ThirdPartyCreditsDialog::OnInitDialog()
{
	SendDlgItemMessage(m_hDlg, IDC_CREDITS, EM_AUTOURLDETECT, AURL_ENABLEURL, NULL);
	SendDlgItemMessage(m_hDlg, IDC_CREDITS, EM_SETEVENTMASK, 0, ENM_LINK);

	std::wstring credits = ResourceHelper::LoadString(GetInstance(), IDS_THIRD_PARTY_CREDITS);
	SetDlgItemText(m_hDlg, IDC_CREDITS, credits.c_str());

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
	std::size_t textLength;

	if (linkNotificationDetails->chrg.cpMax == -1)
	{
		textLength = GetWindowTextLength(linkNotificationDetails->nmhdr.hwndFrom);
	}
	else
	{
		textLength = linkNotificationDetails->chrg.cpMax - linkNotificationDetails->chrg.cpMin;
	}

	std::wstring text(textLength + 1, ' ');

	TEXTRANGE textRange;
	textRange.chrg = linkNotificationDetails->chrg;
	textRange.lpstrText = text.data();
	SendMessage(linkNotificationDetails->nmhdr.hwndFrom, EM_GETTEXTRANGE, 0,
		reinterpret_cast<LPARAM>(&textRange));

	ShellExecute(nullptr, L"open", text.c_str(), nullptr, nullptr, SW_SHOW);
}

INT_PTR ThirdPartyCreditsDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}