// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ShellNavigator.h"
#include "ShellNavigatorFake.h"
#include <gmock/gmock.h>

class ShellNavigatorMock : public ShellNavigator
{
public:
	ShellNavigatorMock();

	MOCK_METHOD(HRESULT, NavigateImpl, (NavigateParams & navigateParams));
	MOCK_METHOD(boost::signals2::connection, AddNavigationStartedObserverImpl,
		(const NavigationStartedSignal::slot_type &observer,
			boost::signals2::connect_position position));
	MOCK_METHOD(boost::signals2::connection, AddNavigationCommittedObserverImpl,
		(const NavigationCommittedSignal::slot_type &observer,
			boost::signals2::connect_position position));
	MOCK_METHOD(boost::signals2::connection, AddNavigationCompletedObserverImpl,
		(const NavigationCompletedSignal::slot_type &observer,
			boost::signals2::connect_position position));
	MOCK_METHOD(boost::signals2::connection, AddNavigationFailedObserverImpl,
		(const NavigationFailedSignal::slot_type &observer,
			boost::signals2::connect_position position));

	HRESULT Navigate(NavigateParams &navigateParams) override;

	boost::signals2::connection AddNavigationStartedObserver(
		const NavigationStartedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationCommittedObserver(
		const NavigationCommittedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationCompletedObserver(
		const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;
	boost::signals2::connection AddNavigationFailedObserver(
		const NavigationFailedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;

private:
	ShellNavigatorFake m_fake;
};
