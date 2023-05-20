// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ThemedDialog.h"
#include "ThemeManager.h"
#include "ThemeWindowTracker.h"

ThemedDialog::ThemedDialog(HINSTANCE resourceInstance, int dialogResourceId, HWND parent,
	DialogSizingType dialogSizingType) :
	BaseDialog(resourceInstance, dialogResourceId, parent, dialogSizingType)
{
}

ThemedDialog::~ThemedDialog() = default;

void ThemedDialog::OnInitDialogBase()
{
	m_themeWindowTracker = std::make_unique<ThemeWindowTracker>(m_hDlg);
}
