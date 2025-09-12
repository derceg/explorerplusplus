// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include <wil/resource.h>

class AboutDialog : public BaseDialog
{
public:
	static AboutDialog *Create(const ResourceLoader *resourceLoader, HWND hParent);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *pnmhdr) override;
	INT_PTR OnClose() override;

private:
	AboutDialog(const ResourceLoader *resourceLoader, HWND hParent);
	~AboutDialog() = default;

	wil::unique_hicon m_icon;
	wil::unique_hicon m_mainIcon;
};
