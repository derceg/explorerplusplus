// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Tab.h"
#include "Config.h"
#include <wil/resource.h>

Tab::Tab(int id, IExplorerplusplus *expp, const FolderSettings *folderSettings,
	boost::optional<FolderColumns> initialColumns) :
	m_id(id),
	m_useCustomName(false),
	m_lockState(LockState::NotLocked)
{
	FolderSettings folderSettingsFinal;

	if (folderSettings)
	{
		folderSettingsFinal = *folderSettings;
	}
	else
	{
		folderSettingsFinal = expp->GetConfig()->defaultFolderSettings;
	}

	m_shellBrowser = CShellBrowser::CreateNew(id, expp->GetLanguageModule(),
		expp->GetMainWindow(), expp->GetCachedIcons(), expp->GetConfig(), folderSettingsFinal,
		initialColumns);

	m_navigationController = std::make_unique<NavigationController>(m_shellBrowser);
}

int Tab::GetId() const
{
	return m_id;
}

NavigationController *Tab::GetNavigationController() const
{
	return m_navigationController.get();
}

CShellBrowser *Tab::GetShellBrowser() const
{
	return m_shellBrowser;
}

// If a custom name has been set, that will be returned. Otherwise, the
// display name of the current directory will be returned.
std::wstring Tab::GetName() const
{
	if (m_useCustomName)
	{
		return m_customName;
	}

	auto pidlDirectory = m_shellBrowser->GetDirectoryIdl();

	TCHAR name[MAX_PATH];
	HRESULT hr = GetDisplayName(pidlDirectory.get(), name, SIZEOF_ARRAY(name), SHGDN_INFOLDER);

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

	m_tabUpdatedSignal(*this, PropertyType::LockState);
}

boost::signals2::connection Tab::AddTabUpdatedObserver(const TabUpdatedSignal::slot_type &observer)
{
	return m_tabUpdatedSignal.connect(observer);
}