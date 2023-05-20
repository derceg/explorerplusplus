// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ThemedDialog.h"
#include "ThemeManager.h"

ThemedDialog::ThemedDialog(HINSTANCE resourceInstance, int dialogResourceId, HWND parent,
	DialogSizingType dialogSizingType) :
	BaseDialog(resourceInstance, dialogResourceId, parent, dialogSizingType)
{
}

void ThemedDialog::OnInitDialogBase()
{
	ThemeManager::GetInstance().ApplyThemeToWindowAndChildren(m_hDlg);
}
