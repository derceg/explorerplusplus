// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationContextMenuController.h"
#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <vector>

class CoreInterface;
class MenuView;
class ThemeManager;

namespace Applications
{

class Application;
class ApplicationExecutor;
class ApplicationModel;

class ApplicationContextMenu : public MenuBase
{
public:
	ApplicationContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		ApplicationModel *model, Application *application, ApplicationExecutor *applicationExecutor,
		CoreInterface *coreInterface, ThemeManager *themeManager);

private:
	void BuildMenu();
	void OnMenuItemSelected(UINT menuItemId);

	ApplicationContextMenuController m_controller;
	std::vector<boost::signals2::scoped_connection> m_connections;
};

}
