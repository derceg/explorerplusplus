// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Tab.h"

Tab::Tab(int id) :
	m_id(id),
	m_locked(false),
	m_addressLocked(false)
{

}

int Tab::GetId() const
{
	return m_id;
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
}

bool Tab::GetLocked() const
{
	return m_locked;
}

void Tab::SetLocked(bool locked)
{
	m_locked = locked;
}

bool Tab::GetAddressLocked() const
{
	return m_addressLocked;
}

void Tab::SetAddressLocked(bool addressLocked)
{
	m_addressLocked = addressLocked;
}