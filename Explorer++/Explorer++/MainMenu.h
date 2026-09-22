// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Icon.h"
#include "MenuBase.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

class AppServices;
class BrowserWindow;
class FeatureList;
class IconFetcher;
class IconModel;
class PidlAbsolute;
class ResourceLoader;
class ShellIconLoader;

class MainMenu : public MenuBase
{
public:
	MainMenu(MenuView *menuView, BrowserWindow *browser, AppServices *appServices,
		ShellIconLoader *shellIconLoader, IconFetcher *iconFetcher);

private:
	void BuildMenu(BrowserWindow *browser, const FeatureList *featureList, AppServices *appServices,
		ShellIconLoader *shellIconLoader, IconFetcher *iconFetcher);
	void BuildFileMenu(const FeatureList *featureList, AppServices *appServices,
		ShellIconLoader *shellIconLoader);
	void BuildEditMenu();
	void BuildSelectionMenu();
	void BuildViewMenu(const FeatureList *featureList);
	void BuildToolbarsSubMenu(MenuView *toolbarsMenuView);
	void BuildFilterSubMenu(MenuView *filterMenuView);
	void BuildActionsMenu();
	void BuildGoMenu(BrowserWindow *browser, AppServices *appServices,
		ShellIconLoader *shellIconLoader);
	void AppendGoMenuItem(MenuView *goMenuView, UINT id, const KNOWNFOLDERID &folderId,
		ShellIconLoader *shellIconLoader);
	void AppendGoMenuItem(MenuView *goMenuView, UINT id, const std::wstring &path,
		ShellIconLoader *shellIconLoader);
	void AppendGoMenuItem(MenuView *goMenuView, UINT id, const PidlAbsolute &pidl,
		ShellIconLoader *shellIconLoader);
	void BuildBookmarksMenu(BrowserWindow *browser, AppServices *appServices,
		IconFetcher *iconFetcher);
	void BuildToolsMenu(const FeatureList *featureList);
	void BuildWindowMenu();
	void BuildHelpMenu();

	void AppendItem(MenuView *menuView, UINT id, UINT textResourceId);
	MenuView *AppendSubMenu(MenuView *menuView, UINT id, UINT textResourceId);
	std::unique_ptr<IconModel> MaybeCreateIconModel(UINT id) const;
	static std::optional<Icon> GetIconForCommand(UINT id);

	std::vector<std::unique_ptr<MenuBase>> m_childMenus;
	const ResourceLoader *const m_resourceLoader;
};
