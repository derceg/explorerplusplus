// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "MenuDelegate.h"

class BookmarkTree;
class BrowserWindow;
class MenuView;
class PlatformContext;
class ResourceLoader;
class TabContainer;
class TabRestorer;

class TabContainerBackgroundContextMenu : public MenuBase, private MenuDelegate
{
public:
	TabContainerBackgroundContextMenu(MenuView *menuView,
		const AcceleratorManager *acceleratorManager, TabContainer *tabContainer,
		TabRestorer *tabRestorer, BookmarkTree *bookmarkTree, BrowserWindow *browser,
		const ResourceLoader *resourceLoader, PlatformContext *platformContext);

private:
	void BuildMenu();

	// MenuDelegate
	bool IsItemEnabled(UINT id) const override;
	void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override;

	TabContainer *const m_tabContainer;
	TabRestorer *const m_tabRestorer;
	BookmarkTree *const m_bookmarkTree;
	BrowserWindow *const m_browser;
	const ResourceLoader *const m_resourceLoader;
	PlatformContext *const m_platformContext;
};
