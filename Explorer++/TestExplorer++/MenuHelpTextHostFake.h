// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/MenuHelpTextHost.h"

class MenuHelpTextHostFake : public MenuHelpTextHost
{
public:
	// MenuHelpTextHost
	void NotifyTopLevelMenuShown() override;
	void NotifyTopLevelMenuClosed() override;
	void SetMenuHelpText(const std::wstring &text) override;

	const std::wstring &GetCurrentHelpText() const;

private:
	std::wstring m_helpText;
};
