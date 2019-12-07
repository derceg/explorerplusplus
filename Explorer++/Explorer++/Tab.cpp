// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Tab.h"
#include "Config.h"
#include <wil/resource.h>

Tab::Tab(int id, IExplorerplusplus *expp, const TabSettings &tabSettings, const FolderSettings *folderSettings,
	boost::optional<FolderColumns> initialColumns) :
	m_id(id),
	m_useCustomName(false),
	m_locked(false),
	m_addressLocked(false)
{
	if (tabSettings.locked)
	{
		SetLocked(*tabSettings.locked);
	}

	if (tabSettings.addressLocked)
	{
		SetAddressLocked(*tabSettings.addressLocked);
	}

	if (tabSettings.name && !tabSettings.name->empty())
	{
		SetCustomName(*tabSettings.name);
	}

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

	m_tabUpdatedSignal(*this, PropertyType::NAME);
}

void Tab::ClearCustomName()
{
	m_useCustomName = false;
	m_customName.erase();

	m_tabUpdatedSignal(*this, PropertyType::NAME);
}

bool Tab::GetLocked() const
{
	return m_locked;
}

void Tab::SetLocked(bool locked)
{
	if (locked == m_locked)
	{
		return;
	}

	m_locked = locked;

	/* The "Lock Tab" and "Lock Tab and Address" options are mutually
	exclusive. */
	if (locked)
	{
		m_addressLocked = false;
	}

	m_tabUpdatedSignal(*this, PropertyType::LOCKED);
}

bool Tab::GetAddressLocked() const
{
	return m_addressLocked;
}

void Tab::SetAddressLocked(bool addressLocked)
{
	if (addressLocked == m_addressLocked)
	{
		return;
	}

	m_addressLocked = addressLocked;

	if (addressLocked)
	{
		m_locked = false;
	}

	m_tabUpdatedSignal(*this, PropertyType::ADDRESS_LOCKED);
}

boost::signals2::connection Tab::AddTabUpdatedObserver(const TabUpdatedSignal::slot_type &observer)
{
	return m_tabUpdatedSignal.connect(observer);
}