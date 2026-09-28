// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"

class BrowserWindow;
struct Config;
class ResourceLoader;

class DisplayWindowContextMenu : public MenuBase
{
public:
	DisplayWindowContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BrowserWindow *browser, Config *config, const ResourceLoader *resourceLoader);

private:
	void BuildMenu(const ResourceLoader *resourceLoader);
	void OnChangeColors();

	BrowserWindow *const m_browser;
	Config *const m_config;
};
