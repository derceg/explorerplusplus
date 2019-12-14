// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationToolbar.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"

class CApplicationToolbarButtonDialog : public CBaseDialog
{
public:

	CApplicationToolbarButtonDialog(HINSTANCE hInstance, HWND hParent, ApplicationButton_t *Button, bool IsNew);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();

private:

	void	OnChooseFile();

	void	OnOk();
	void	OnCancel();

	ApplicationButton_t	*m_Button;
	bool				m_IsNew;
};