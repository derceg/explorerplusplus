// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationContextMenuController.h"

class CoreInterface;

namespace Applications
{

class Application;
class ApplicationModel;

class ApplicationContextMenu
{
public:
	ApplicationContextMenu(ApplicationModel *model, CoreInterface *coreInterface);

	void ShowMenu(HWND parentWindow, Application *application, const POINT &ptScreen);

private:
	HMODULE m_resourceModule;
	ApplicationModel *m_model;
	ApplicationContextMenuController m_controller;
};

}
