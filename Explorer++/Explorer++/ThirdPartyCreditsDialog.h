// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"

class ThirdPartyCreditsDialog : public BaseDialog
{
public:
	ThirdPartyCreditsDialog(HINSTANCE instance, HWND parent);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *pnmhdr) override;
	INT_PTR OnClose() override;

private:
	INT_PTR OnLinkNotification(const ENLINK *linkNotificationDetails);

	void OnLinkClicked(const ENLINK *linkNotificationDetails);
};