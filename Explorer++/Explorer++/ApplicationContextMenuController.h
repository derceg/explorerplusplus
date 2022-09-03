// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class CoreInterface;

namespace Applications
{

class ApplicationModel;
class Application;

class ApplicationContextMenuController
{
public:
	ApplicationContextMenuController(CoreInterface *coreInterface);

	void OnMenuItemSelected(int menuItemId, ApplicationModel *model, Application *targetApplication,
		size_t targetIndex, HWND parentWindow);

private:
	void OnOpen(HWND parentWindow, const Application *targetApplication);
	void OnShowProperties(HWND parentWindow, ApplicationModel *model,
		Application *targetApplication);
	void OnDelete(ApplicationModel *model, const Application *targetApplication, HWND parentWindow);
	void OnNew(HWND parentWindow, ApplicationModel *model, size_t index);

	CoreInterface *m_coreInterface;
};

}
