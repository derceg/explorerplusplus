// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "NavigationManager.h"
#include "ShellNavigationController.h"

ShellBrowser::ShellBrowser() : m_id(idCounter++)
{
}

int ShellBrowser::GetId() const
{
	return m_id;
}

const Tab *ShellBrowser::GetTab() const
{
	CHECK(m_tab);
	return m_tab;
}

void ShellBrowser::SetTab(const Tab *tab)
{
	CHECK(!m_tab);
	m_tab = tab;
}

const PidlAbsolute &ShellBrowser::GetDirectory() const
{
	const auto *currentEntry = GetNavigationController()->GetCurrentEntry();
	return currentEntry->GetPidl();
}

const NavigationRequest *ShellBrowser::MaybeGetLatestActiveNavigation() const
{
	return GetNavigationManager()->MaybeGetLatestActiveNavigation();
}
