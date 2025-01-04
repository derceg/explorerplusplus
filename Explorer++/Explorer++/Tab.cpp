// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Tab.h"
#include "Config.h"
#include "CoreInterface.h"
#include "PreservedTab.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabStorage.h"
#include <wil/resource.h>

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
Tab::Tab(std::shared_ptr<ShellBrowser> shellBrowser, BrowserWindow *browser) :
	m_id(idCounter++),
	m_shellBrowser(shellBrowser),
	m_shellBrowserImpl(std::dynamic_pointer_cast<ShellBrowserImpl>(shellBrowser)),
	m_browser(browser),
	m_useCustomName(false),
	m_lockState(LockState::NotLocked)
{
	// The provided ShellBrowser instance may be null in tests.
	if (m_shellBrowserImpl)
	{
		m_shellBrowserImpl->SetID(m_id);
	}
}

Tab::Tab(const PreservedTab &preservedTab, std::shared_ptr<ShellBrowser> shellBrowser,
	BrowserWindow *browser) :
	m_id(idCounter++),
	m_shellBrowser(shellBrowser),
	m_shellBrowserImpl(std::dynamic_pointer_cast<ShellBrowserImpl>(shellBrowser)),
	m_browser(browser),
	m_useCustomName(preservedTab.useCustomName),
	m_customName(preservedTab.customName),
	m_lockState(preservedTab.lockState)
{
	if (m_shellBrowserImpl)
	{
		m_shellBrowserImpl->SetID(m_id);
	}
}

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
	return m_shellBrowserImpl.get();
}

std::weak_ptr<ShellBrowserImpl> Tab::GetShellBrowserImplWeak() const
{
	return std::weak_ptr<ShellBrowserImpl>(m_shellBrowserImpl);
}

BrowserWindow *Tab::GetBrowser() const
{
	return m_browser;
}

// If a custom name has been set, that will be returned. Otherwise, the
// display name of the current directory will be returned.
std::wstring Tab::GetName() const
{
	if (m_useCustomName)
	{
		return m_customName;
	}

	auto *entry = m_shellBrowser->GetNavigationController()->GetCurrentEntry();

	std::wstring name;
	HRESULT hr = GetDisplayName(entry->GetPidl().Raw(), SHGDN_INFOLDER, name);

	if (FAILED(hr))
	{
		return L"(Unknown)";
	}

	return name;
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

	m_tabUpdatedSignal(*this, PropertyType::Name);
}

void Tab::ClearCustomName()
{
	m_useCustomName = false;
	m_customName.erase();

	m_tabUpdatedSignal(*this, PropertyType::Name);
}

Tab::LockState Tab::GetLockState() const
{
	return m_lockState;
}

void Tab::SetLockState(LockState lockState)
{
	if (lockState == m_lockState)
	{
		return;
	}

	m_lockState = lockState;

	NavigationMode navigationMode = (lockState == LockState::AddressLocked)
		? NavigationMode::ForceNewTab
		: NavigationMode::Normal;
	m_shellBrowser->GetNavigationController()->SetNavigationMode(navigationMode);

	m_tabUpdatedSignal(*this, PropertyType::LockState);
}

boost::signals2::connection Tab::AddTabUpdatedObserver(const TabUpdatedSignal::slot_type &observer)
{
	return m_tabUpdatedSignal.connect(observer);
}

TabStorageData Tab::GetStorageData() const
{
	// The ShellBrowser instance can be null in tests, in which case, this method shouldn't be
	// called.
	CHECK(m_shellBrowserImpl);

	TabStorageData storageData;
	storageData.pidl = m_shellBrowserImpl->GetDirectoryIdl().get();
	storageData.directory = m_shellBrowserImpl->GetDirectory();
	storageData.folderSettings = m_shellBrowserImpl->GetFolderSettings();
	storageData.columns = m_shellBrowserImpl->ExportAllColumns();

	TabSettings tabSettings;

	if (m_useCustomName)
	{
		tabSettings.name = m_customName;
	}

	tabSettings.lockState = m_lockState;

	storageData.tabSettings = tabSettings;

	return storageData;
}
