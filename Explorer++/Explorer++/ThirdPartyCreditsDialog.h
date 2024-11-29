// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ThemedDialog.h"

class ThirdPartyCreditsDialog : public ThemedDialog
{
public:
	ThirdPartyCreditsDialog(HINSTANCE resourceInstance, HWND parent, ThemeManager *themeManager);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *pnmhdr) override;
	INT_PTR OnClose() override;

private:
	INT_PTR OnLinkNotification(const ENLINK *linkNotificationDetails);

	void OnLinkClicked(const ENLINK *linkNotificationDetails);
};
