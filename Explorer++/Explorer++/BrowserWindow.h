// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Navigator.h"
#include <boost/signals2.hpp>

class BrowserCommandController;
class BrowserPane;
struct PreservedTab;
class ShellBrowser;
struct WindowStorageData;

// Each browser window contains one or more browser panes, with each pane containing a set of tabs.
class BrowserWindow : public Navigator
{
public:
	using BrowserInitializedSignal = boost::signals2::signal<void()>;

	virtual ~BrowserWindow() = default;

	virtual int GetId() const = 0;
	virtual boost::signals2::connection AddBrowserInitializedObserver(
		const BrowserInitializedSignal::slot_type &observer) = 0;
	virtual BrowserCommandController *GetCommandController() = 0;
	virtual BrowserPane *GetActivePane() const = 0;
	virtual void FocusActiveTab() = 0;
	virtual void CreateTabFromPreservedTab(PreservedTab *tab) = 0;
	virtual ShellBrowser *GetActiveShellBrowser() = 0;
	virtual HWND GetHWND() const = 0;
	virtual WindowStorageData GetStorageData() const = 0;
	virtual bool IsActive() const = 0;
	virtual void Activate() = 0;
	virtual void TryClose() = 0;
	virtual void Close() = 0;
};
