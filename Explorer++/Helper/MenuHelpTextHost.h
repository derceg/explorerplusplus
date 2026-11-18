// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

// Represents a UI component that can ultimately display menu help text.
class MenuHelpTextHost
{
public:
	virtual ~MenuHelpTextHost() = default;

	virtual void NotifyTopLevelMenuShown() = 0;
	virtual void NotifyTopLevelMenuClosed() = 0;

	// Called when a menu item is highlighted and the help text needs to be shown.
	virtual void SetMenuHelpText(const std::wstring &text) = 0;
};
