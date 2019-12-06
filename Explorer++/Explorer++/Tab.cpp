// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Tab.h"
#include <wil/resource.h>

Tab::Tab(int id) :
	m_id(id),
	m_useCustomName(false),
	m_locked(false),
	m_addressLocked(false)
{

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

/* TODO: Ideally, this method wouldn't exist (the value would be set
during construction of the tab object). */
void Tab::SetShellBrowser(CShellBrowser *shellBrowser)
{
	m_shellBrowser = shellBrowser;

	m_navigationController = std::make_unique<NavigationController>(shellBrowser);
}

HWND Tab::GetListView() const
{
	return m_listView;
}

void Tab::SetListView(HWND listView)
{
	m_listView = listView;
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