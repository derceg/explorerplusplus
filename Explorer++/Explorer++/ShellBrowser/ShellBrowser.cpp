// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "NavigationManager.h"
#include "ShellBrowserHelper.h"

void ShellBrowser::AddHelper(std::unique_ptr<ShellBrowserHelperBase> helper)
{
	m_helpers.push_back(std::move(helper));
}

const NavigateParams *ShellBrowser::MaybeGetLatestActiveNavigation() const
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
	return GetNavigationManager()->AddNavigationCommittedObserver(observer, position);
}

boost::signals2::connection ShellBrowser::AddNavigationCompletedObserver(
	const NavigationCompletedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return GetNavigationManager()->AddNavigationCompletedObserver(observer, position);
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
