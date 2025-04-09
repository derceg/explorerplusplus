// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ThemedDialog.h"
#include "ThemeManager.h"
#include "ThemeWindowTracker.h"

ThemedDialog::ThemedDialog(const ResourceLoader *resourceLoader, HINSTANCE resourceInstance,
	int dialogResourceId, HWND parent, DialogSizingType dialogSizingType,
	ThemeManager *themeManager) :
	BaseDialog(resourceLoader, resourceInstance, dialogResourceId, parent, dialogSizingType),
	m_themeManager(themeManager)
{
}

ThemedDialog::~ThemedDialog() = default;

void ThemedDialog::OnInitDialogBase()
{
	m_themeWindowTracker = std::make_unique<ThemeWindowTracker>(m_hDlg, m_themeManager);
}

ThemeManager *ThemedDialog::GetThemeManager() const
{
	return m_themeManager;
}
