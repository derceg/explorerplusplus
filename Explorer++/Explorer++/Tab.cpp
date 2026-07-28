// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Tab.h"
#include "Config.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabEvents.h"
#include "TabStorage.h"
#include "TerminalHost.h"

Tab::Tab(std::unique_ptr<ShellBrowser> shellBrowser, BrowserWindow *browser,
	TabContainer *tabContainer, TabEvents *tabEvents) :
	Tab(std::move(shellBrowser), browser, tabContainer, tabEvents, {})
{
}

// Note the use of std::dynamic_pointer_cast below. There are a number of places where the caller
// needs to be able to retrieve the ShellBrowser implementation class (ShellBrowserImpl), so that's
// something this class has to provide.
// In tests, however, it's not feasible to provide the concrete implementation instance. While the
// instance can be set to null (and is in some tests), doing that greatly limits the way in which
// this class can be used when testing. In turn, that can make it infeasible to test classes that
// rely on this class.
// By accepting a ShellBrowser instance and dynamically casting it to ShellBrowserImpl, this class
// can continue to provide access to the implementation in normal circumstances (outside of tests).
// Within tests, the implementation won't be available, however GetShellBrowser() will still work
// (provided the test has supplied a ShellBrowser instance). That's useful, since other classes may
// only need to access the ShellBrowser through its base interface, rather than via the
// implementation class.
// If the ShellBrowser interface expands to cover all the necessary functionality, or
// ShellBrowserImpl is simplified enough to make it usable in tests, the casting here can be
// removed.
Tab::Tab(std::unique_ptr<ShellBrowser> shellBrowser, BrowserWindow *browser,
	TabContainer *tabContainer, TabEvents *tabEvents, const InitialData &initialData) :
	m_id(idCounter++),
	m_shellBrowser(std::move(shellBrowser)),
	m_shellBrowserImpl(dynamic_cast<ShellBrowserImpl *>(m_shellBrowser.get())),
	m_browser(browser),
	m_tabContainer(tabContainer),
	m_tabEvents(tabEvents),
	m_useCustomName(initialData.useCustomName),
	m_customName(initialData.customName),
	m_lockState(LockState::NotLocked)
{
	m_shellBrowser->SetTab(this);

	ApplyLockState(initialData.lockState, NotificationMode::DontNotify);
}

Tab::~Tab() = default;

int Tab::GetId() const
{
	return m_id;
}

ShellBrowser *Tab::GetShellBrowser() const
{
	return m_shellBrowser.get();
}

ShellBrowserImpl *Tab::GetShellBrowserImpl() const
{
	return m_shellBrowserImpl;
}

bool Tab::IsTerminal() const
{
	return m_terminalHost != nullptr;
}

void Tab::SetTerminalHost(std::unique_ptr<TerminalHost> terminalHost)
{
	CHECK(!m_terminalHost);
	CHECK(terminalHost);

	HWND listView = m_shellBrowserImpl->GetListView();
	RECT bounds{};
	GetWindowRect(listView, &bounds);
	MapWindowPoints(nullptr, GetParent(terminalHost->GetHWND()), reinterpret_cast<POINT *>(&bounds),
		2);

	terminalHost->SetBounds(bounds.left, bounds.top, bounds.right - bounds.left,
		bounds.bottom - bounds.top, false);
	terminalHost->SetDirectoryChangedCallback(
		std::bind_front(&Tab::OnTerminalDirectoryChanged, this));
	ShowWindow(listView, SW_HIDE);
	m_terminalHost = std::move(terminalHost);
	m_tabEvents->NotifyUpdated(*this, PropertyType::Name);
}

void Tab::SetTerminalFontSize(int fontSize)
{
	if (m_terminalHost)
	{
		m_terminalHost->SetFontSize(fontSize);
	}
}

HWND Tab::GetContentWindow() const
{
	if (m_terminalHost)
	{
		return m_terminalHost->GetHWND();
	}

	return m_shellBrowserImpl->GetListView();
}

void Tab::SetContentBounds(int x, int y, int width, int height, bool visible)
{
	if (m_terminalHost)
	{
		m_terminalHost->SetBounds(x, y, width, height, visible);
		return;
	}

	UINT flags = SWP_NOZORDER | (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
	SetWindowPos(m_shellBrowserImpl->GetListView(), nullptr, x, y, width, height, flags);
}

void Tab::FocusContent() const
{
	if (m_terminalHost)
	{
		m_terminalHost->Focus();
		return;
	}

	SetFocus(m_shellBrowserImpl->GetListView());
}

std::optional<std::wstring> Tab::GetTerminalDirectory() const
{
	return m_terminalDirectory;
}

BrowserWindow *Tab::GetBrowser() const
{
	return m_browser;
}

TabContainer *Tab::GetTabContainer() const
{
	return m_tabContainer;
}

// If a custom name has been set, that will be returned. Otherwise, the
// display name of the current directory will be returned.
std::wstring Tab::GetName() const
{
	if (m_terminalDirectory)
	{
		auto path = std::filesystem::path(*m_terminalDirectory);

		while (!path.has_filename() && path != path.root_path())
		{
			path = path.parent_path();
		}

		auto name = path.filename().wstring();
		return name.empty() ? path.root_path().wstring() : name;
	}

	if (m_useCustomName)
	{
		return m_customName;
	}

	auto *entry = m_shellBrowser->GetNavigationController()->GetCurrentEntry();
	return GetDisplayNameWithFallback(entry->GetPidl().Raw(), SHGDN_INFOLDER);
}

void Tab::OnTerminalDirectoryChanged(const std::wstring &directory)
{
	if (directory.empty() || m_terminalDirectory == directory)
	{
		return;
	}

	m_terminalDirectory = directory;
	m_tabEvents->NotifyUpdated(*this, PropertyType::Name);
}

bool Tab::GetUseCustomName() const
{
	return m_useCustomName;
}

void Tab::SetCustomName(const std::wstring &name)
{
	if (name.empty())
	{
		return;
	}

	m_useCustomName = true;
	m_customName = name;

	m_tabEvents->NotifyUpdated(*this, PropertyType::Name);
}

void Tab::ClearCustomName()
{
	m_useCustomName = false;
	m_customName.erase();

	m_tabEvents->NotifyUpdated(*this, PropertyType::Name);
}

Tab::LockState Tab::GetLockState() const
{
	return m_lockState;
}

void Tab::SetLockState(LockState lockState)
{
	ApplyLockState(lockState, NotificationMode::Notify);
}

void Tab::ApplyLockState(LockState lockState, NotificationMode notificationMode)
{
	if (lockState == m_lockState)
	{
		return;
	}

	m_lockState = lockState;

	auto navigationTargetMode = (lockState == LockState::AddressLocked)
		? NavigationTargetMode::ForceNewTab
		: NavigationTargetMode::Normal;
	m_shellBrowser->GetNavigationController()->SetNavigationTargetMode(navigationTargetMode);

	if (notificationMode == NotificationMode::Notify)
	{
		m_tabEvents->NotifyUpdated(*this, PropertyType::LockState);
	}
}

bool Tab::IsLocked() const
{
	return m_lockState == LockState::Locked || m_lockState == LockState::AddressLocked;
}

TabStorageData Tab::GetStorageData() const
{
	// The ShellBrowser instance can be null in tests, in which case, this method shouldn't be
	// called.
	CHECK(m_shellBrowserImpl);

	TabStorageData storageData;
	storageData.pidl = m_shellBrowserImpl->GetDirectoryIdl().get();
	storageData.directory = m_shellBrowserImpl->GetDirectoryPath();
	storageData.folderSettings = m_shellBrowserImpl->GetFolderSettings();
	storageData.columns = m_shellBrowserImpl->GetAllColumnSets();

	TabSettings tabSettings;

	if (m_useCustomName)
	{
		tabSettings.name = m_customName;
	}

	tabSettings.lockState = m_lockState;

	storageData.tabSettings = tabSettings;

	return storageData;
}
