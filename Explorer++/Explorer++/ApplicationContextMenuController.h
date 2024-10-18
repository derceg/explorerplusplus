// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class CoreInterface;

namespace Applications
{

class Application;
class ApplicationExecutor;
class ApplicationModel;

class ApplicationContextMenuController
{
public:
	ApplicationContextMenuController(ApplicationModel *model, Application *application,
		ApplicationExecutor *applicationExecutor, CoreInterface *coreInterface);

	void OnMenuItemSelected(UINT menuItemId);

private:
	void OnOpen();
	void OnNew();
	void OnDelete();
	void OnShowProperties();

	ApplicationModel *const m_model;
	Application *const m_application;
	ApplicationExecutor *const m_applicationExecutor;
	CoreInterface *const m_coreInterface;
};

}
