// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <memory>

class BrowserWindow;
class ShellBrowser;
class ShellBrowserImpl;
class TabContainer;
class TabEvents;
struct TabStorageData;

class Tab : private boost::noncopyable
{
public:
	enum class PropertyType
	{
		Name,
		LockState
	};

	enum class LockState
	{
		// The tab isn't locked; it can be navigated freely and closed.
		NotLocked,

		// The tab is locked. It can be navigated freely, but not closed.
		Locked,

		// Both the tab and address are locked. The tab can't be navigated or closed. All
		// navigations will proceed in a new tab.
		AddressLocked
	};

	struct InitialData
	{
		bool useCustomName = false;
		std::wstring customName;
		LockState lockState = LockState::NotLocked;
	};

	Tab(std::unique_ptr<ShellBrowser> shellBrowser, BrowserWindow *browser,
		TabContainer *tabContainer, TabEvents *tabEvents, const InitialData &initialData = {});

	int GetId() const;

	ShellBrowser *GetShellBrowser() const;
	ShellBrowserImpl *GetShellBrowserImpl() const;

	BrowserWindow *GetBrowser() const;
	TabContainer *GetTabContainer() const;

	std::wstring GetName() const;
	bool GetUseCustomName() const;
	void SetCustomName(const std::wstring &name);
	void ClearCustomName();

	LockState GetLockState() const;
	void SetLockState(LockState lockState);

	// Returns true if the tab is locked, or address locked.
	bool IsLocked() const;

	TabStorageData GetStorageData() const;

	/* Although each tab manages its
	own columns, it does not know
	about any column defaults.
	Therefore, it makes more sense
	for this setting to remain here. */
	// BOOL	bUsingDefaultColumns;

private:
	enum class NotificationMode
	{
		DontNotify,
		Notify
	};

	void ApplyLockState(LockState lockState, NotificationMode notificationMode);

	static inline int idCounter = 1;
	const int m_id;

	const std::unique_ptr<ShellBrowser> m_shellBrowser;
	ShellBrowserImpl *const m_shellBrowserImpl;

	BrowserWindow *const m_browser;
	TabContainer *const m_tabContainer;
	TabEvents *const m_tabEvents;

	bool m_useCustomName;
	std::wstring m_customName;
	LockState m_lockState;
};
