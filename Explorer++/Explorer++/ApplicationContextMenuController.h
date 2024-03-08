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
	ApplicationContextMenuController(ApplicationExecutor *applicationExecutor,
		CoreInterface *coreInterface);

	void OnMenuItemSelected(int menuItemId, ApplicationModel *model, Application *targetApplication,
		size_t targetIndex, HWND parentWindow);

private:
	void OnOpen(const Application *targetApplication);
	void OnShowProperties(HWND parentWindow, ApplicationModel *model,
		Application *targetApplication);
	void OnDelete(ApplicationModel *model, const Application *targetApplication, HWND parentWindow);
	void OnNew(HWND parentWindow, ApplicationModel *model, size_t index);

	ApplicationExecutor *m_applicationExecutor = nullptr;
	CoreInterface *m_coreInterface = nullptr;
};

}
