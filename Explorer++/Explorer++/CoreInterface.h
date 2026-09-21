// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ClipboardHelper.h"

class ShellBrowserImpl;
class TabContainer;

// Deprecated interface between Explorerplusplus and some of the other components (such as the
// dialogs and toolbars).
class CoreInterface
{
public:
	virtual ~CoreInterface() = default;

	virtual ShellBrowserImpl *GetActiveShellBrowserImpl() const = 0;

	virtual TabContainer *GetTabContainer() const = 0;

	virtual BOOL CanPaste(PasteType pasteType) const = 0;
};
