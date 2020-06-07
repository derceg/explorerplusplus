// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationToolbar.h"
#include "DarkModeDialogBase.h"

class ApplicationToolbarButtonDialog : public DarkModeDialogBase
{
public:
	ApplicationToolbarButtonDialog(
		HINSTANCE hInstance, HWND hParent, ApplicationButton *Button, bool IsNew);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

private:
	void OnChooseFile();

	void OnOk();
	void OnCancel();

	ApplicationButton *m_Button;
	bool m_IsNew;
};