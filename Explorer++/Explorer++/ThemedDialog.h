// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"

class ThemeWindowTracker;

class ThemedDialog : public BaseDialog
{
public:
	ThemedDialog(HINSTANCE resourceInstance, int dialogResourceId, HWND parent,
		DialogSizingType dialogSizingType);
	~ThemedDialog();

private:
	void OnInitDialogBase() override final;

	std::unique_ptr<ThemeWindowTracker> m_themeWindowTracker;
};
