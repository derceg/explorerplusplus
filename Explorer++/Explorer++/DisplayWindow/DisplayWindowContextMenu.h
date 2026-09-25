// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "MenuDelegate.h"

class BrowserWindow;
struct Config;
class ResourceLoader;

class DisplayWindowContextMenu : public MenuBase, private MenuDelegate
{
public:
	DisplayWindowContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BrowserWindow *browser, Config *config, const ResourceLoader *resourceLoader);

private:
	void BuildMenu();

	// MenuDelegate
	bool IsItemChecked(UINT id) const override;
	void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override;

	BrowserWindow *const m_browser;
	Config *const m_config;
	const ResourceLoader *const m_resourceLoader;
};
