// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Tab.h"

Tab::Tab(int id) :
	id(id),
	m_locked(false),
	m_addressLocked(false)
{

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