// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellNavigatorMock.h"

ShellNavigatorMock::ShellNavigatorMock()
{
	ON_CALL(*this, NavigateImpl)
		.WillByDefault(
			[this](NavigateParams &navigateParams) { return m_fake.Navigate(navigateParams); });

	ON_CALL(*this, AddNavigationStartedObserverImpl)
		.WillByDefault([this](const NavigationStartedSignal::slot_type &observer,
						   boost::signals2::connect_position position)
			{ return m_fake.AddNavigationStartedObserver(observer, position); });

	ON_CALL(*this, AddNavigationCommittedObserverImpl)
		.WillByDefault([this](const NavigationCommittedSignal::slot_type &observer,
						   boost::signals2::connect_position position)
			{ return m_fake.AddNavigationCommittedObserver(observer, position); });

	ON_CALL(*this, AddNavigationCompletedObserverImpl)
		.WillByDefault([this](const NavigationCompletedSignal::slot_type &observer,
						   boost::signals2::connect_position position)
			{ return m_fake.AddNavigationCompletedObserver(observer, position); });

	ON_CALL(*this, AddNavigationFailedObserverImpl)
		.WillByDefault([this](const NavigationFailedSignal::slot_type &observer,
						   boost::signals2::connect_position position)
			{ return m_fake.AddNavigationFailedObserver(observer, position); });
}

HRESULT ShellNavigatorMock::Navigate(NavigateParams &navigateParams)
{
	return NavigateImpl(navigateParams);
}

boost::signals2::connection ShellNavigatorMock::AddNavigationStartedObserver(
	const NavigationStartedSignal::slot_type &observer, boost::signals2::connect_position position)
{
	return AddNavigationStartedObserver(observer, position);
}

boost::signals2::connection ShellNavigatorMock::AddNavigationCommittedObserver(
	const NavigationCommittedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return AddNavigationCommittedObserverImpl(observer, position);
}

boost::signals2::connection ShellNavigatorMock::AddNavigationCompletedObserver(
	const NavigationCompletedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return AddNavigationCompletedObserverImpl(observer, position);
}

boost::signals2::connection ShellNavigatorMock::AddNavigationFailedObserver(
	const NavigationFailedSignal::slot_type &observer, boost::signals2::connect_position position)
{
	return AddNavigationFailedObserverImpl(observer, position);
}
