// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "NavigationManager.h"
#include "ShellBrowserHelper.h"

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

void ShellBrowser::AddHelper(std::unique_ptr<ShellBrowserHelperBase> helper)
{
	m_helpers.push_back(std::move(helper));
}

const NavigationRequest *ShellBrowser::MaybeGetLatestActiveNavigation() const
{
	return GetNavigationManager()->MaybeGetLatestActiveNavigation();
}

boost::signals2::connection ShellBrowser::AddNavigationStartedObserver(
	const NavigationStartedSignal::slot_type &observer, boost::signals2::connect_position position)
{
	return GetNavigationManager()->AddNavigationStartedObserver(observer, position);
}

boost::signals2::connection ShellBrowser::AddNavigationCommittedObserver(
	const NavigationCommittedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return GetNavigationManager()->AddNavigationCommittedObserver(
		[observer](const NavigationRequest *request, const std::vector<PidlChild> &items)
		{
			UNREFERENCED_PARAMETER(items);

			observer(request);
		},
		position);
}

boost::signals2::connection ShellBrowser::AddNavigationFailedObserver(
	const NavigationFailedSignal::slot_type &observer, boost::signals2::connect_position position)
{
	return GetNavigationManager()->AddNavigationFailedObserver(observer, position);
}

boost::signals2::connection ShellBrowser::AddNavigationCancelledObserver(
	const NavigationCancelledSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return GetNavigationManager()->AddNavigationCancelledObserver(observer, position);
}
