// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "MenuDelegate.h"

class BrowserWindow;
class MenuView;
class ResourceLoader;

namespace Applications
{

class Application;
class ApplicationExecutor;
class ApplicationModel;

class ApplicationContextMenu : public MenuBase, private MenuDelegate
{
public:
	ApplicationContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		ApplicationModel *model, Application *application, ApplicationExecutor *applicationExecutor,
		const BrowserWindow *browser, const ResourceLoader *resourceLoader);

private:
	void BuildMenu();

	// MenuDelegate
	void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override;

	void OnOpen();
	void OnNew();
	void OnDelete();
	void OnShowProperties();

	ApplicationModel *const m_model;
	Application *const m_application;
	ApplicationExecutor *const m_applicationExecutor;
	const BrowserWindow *const m_browser;
	const ResourceLoader *const m_resourceLoader;
};

}
