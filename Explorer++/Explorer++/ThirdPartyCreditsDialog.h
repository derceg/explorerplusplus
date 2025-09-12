// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"

class ThirdPartyCreditsDialog : public BaseDialog
{
public:
	static ThirdPartyCreditsDialog *Create(const ResourceLoader *resourceLoader, HWND parent);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *pnmhdr) override;
	INT_PTR OnClose() override;

private:
	ThirdPartyCreditsDialog(const ResourceLoader *resourceLoader, HWND parent);
	~ThirdPartyCreditsDialog() = default;

	INT_PTR OnLinkNotification(const ENLINK *linkNotificationDetails);

	void OnLinkClicked(const ENLINK *linkNotificationDetails);
};
