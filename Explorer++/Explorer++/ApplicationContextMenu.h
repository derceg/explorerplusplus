// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationContextMenuController.h"

class CoreInterface;

namespace Applications
{

class Application;
class ApplicationExecutor;
class ApplicationModel;

class ApplicationContextMenu
{
public:
	ApplicationContextMenu(ApplicationModel *model, ApplicationExecutor *applicationExecutor,
		CoreInterface *coreInterface);

	void ShowMenu(HWND parentWindow, Application *application, const POINT &ptScreen);

private:
	HINSTANCE m_resourceInstance;
	ApplicationModel *m_model;
	ApplicationContextMenuController m_controller;
};

}
