// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellNavigator.h"
#include <memory>
#include <vector>

struct FolderSettings;
class NavigationManager;
class ShellBrowserHelperBase;
class ShellNavigationController;

class ShellBrowser : public ShellNavigator
{
public:
	virtual FolderSettings GetFolderSettings() const = 0;
	virtual ShellNavigationController *GetNavigationController() const = 0;

	void AddHelper(std::unique_ptr<ShellBrowserHelperBase> helper);

	// ShellNavigator
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

protected:
	virtual NavigationManager *GetNavigationManager() = 0;

private:
	std::vector<std::unique_ptr<ShellBrowserHelperBase>> m_helpers;
};
