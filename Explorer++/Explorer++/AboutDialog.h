// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
#include <wil/resource.h>

class AboutDialog : public BaseDialog
{
public:

	AboutDialog(HINSTANCE hInstance, HWND hParent);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnNotify(NMHDR *pnmhdr);
	INT_PTR	OnClose();

private:

	wil::unique_hicon m_icon;
	wil::unique_hicon m_mainIcon;
};