// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Navigator.h"
#include "../Helper/MenuHelpTextRequest.h"
#include <boost/signals2.hpp>

class BrowserCommandController;
class BrowserPane;
struct PreservedTab;
class ShellBrowser;
struct WindowStorageData;

// Each browser window contains one or more browser panes, with each pane containing a set of tabs.
class BrowserWindow : public Navigator, public MenuHelpTextRequest
{
public:
	enum class LifecycleState
	{
		// Indicates that the browser is in the process of initializing.
		Starting,

		// Indicates that the browser is in the main part of its lifecycle. That is, it has fully
		// initialized and hasn't started closing.
		Main,

		// Indicates that the browser has started closing.
		Closing
	};

	using LifecycleStateChangedSignal = boost::signals2::signal<void(LifecycleState updatedState)>;

	BrowserWindow();
	virtual ~BrowserWindow() = default;

	int GetId() const;

	LifecycleState GetLifecycleState() const;
	boost::signals2::connection AddLifecycleStateChangedObserver(
		const LifecycleStateChangedSignal::slot_type &observer);

	virtual HWND GetHWND() const = 0;
	virtual BrowserCommandController *GetCommandController() = 0;

	virtual BrowserPane *GetActivePane() const = 0;
	virtual void FocusActiveTab() = 0;
	virtual void CreateTabFromPreservedTab(const PreservedTab *tab) = 0;

	virtual ShellBrowser *GetActiveShellBrowser() = 0;
	virtual const ShellBrowser *GetActiveShellBrowser() const = 0;
	bool IsShellBrowserActive(const ShellBrowser *shellBrowser) const;

	virtual void StartMainToolbarCustomization() = 0;

	virtual std::optional<std::wstring> RequestMenuHelpText(HMENU menu, UINT id) const = 0;

	virtual WindowStorageData GetStorageData() const = 0;

	virtual bool IsActive() const = 0;
	virtual void Activate() = 0;

	virtual void FocusChanged() = 0;

	virtual void TryClose() = 0;
	virtual void Close() = 0;

protected:
	void SetLifecycleState(LifecycleState state);

private:
	static inline int idCounter = 1;
	const int m_id;

	LifecycleState m_lifecycleState = LifecycleState::Starting;
	LifecycleStateChangedSignal m_lifecycleStateChangedSignal;
};
