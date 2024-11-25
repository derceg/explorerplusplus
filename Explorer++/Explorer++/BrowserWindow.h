// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Navigator.h"

class BrowserCommandController;
class BrowserPane;
class ShellBrowser;
struct WindowStorageData;

// Each browser window contains one or more browser panes, with each pane containing a set of tabs.
class BrowserWindow : public Navigator
{
public:
	virtual ~BrowserWindow() = default;

	virtual BrowserCommandController *GetCommandController() = 0;
	virtual BrowserPane *GetActivePane() const = 0;
	virtual void FocusActiveTab() = 0;
	virtual ShellBrowser *GetActiveShellBrowser() = 0;
	virtual HWND GetHWND() const = 0;
	virtual WindowStorageData GetStorageData() const = 0;
	virtual bool IsActive() const = 0;
	virtual void Activate() = 0;
};
