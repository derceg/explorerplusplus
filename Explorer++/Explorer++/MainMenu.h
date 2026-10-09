// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Icon.h"
#include "MenuBase.h"
#include "MenuItemCallbacks.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

class AppServices;
class BrowserWindow;
class ClipboardStore;
struct Config;
class CoreInterface;
class FeatureList;
class FileActionHandler;
class IconFetcher;
class IconModel;
class PidlAbsolute;
class ResourceLoader;
class ShellIconLoader;
class TabRestorer;

class MainMenu : public MenuBase
{
public:
	MainMenu(MenuView *menuView, BrowserWindow *browser, AppServices *appServices,
		ShellIconLoader *shellIconLoader, IconFetcher *iconFetcher,
		const FileActionHandler *fileActionHandler, CoreInterface *coreInterface);

private:
	void BuildMenu(BrowserWindow *browser, AppServices *appServices,
		ShellIconLoader *shellIconLoader, IconFetcher *iconFetcher);
	void BuildFileMenu(TabRestorer *tabRestorer, const FeatureList *featureList,
		ShellIconLoader *shellIconLoader);
	void BuildEditMenu();
	void BuildSelectionMenu();
	void BuildViewMenu(BrowserWindow *browser, AppServices *appServices);
	void BuildFilterSubMenu(MenuView *filterMenuView);
	void BuildActionsMenu();
	void BuildGoMenu(BrowserWindow *browser, AppServices *appServices,
		ShellIconLoader *shellIconLoader);
	void AppendGoMenuItem(MenuView *goMenuView, WORD id, const KNOWNFOLDERID &folderId,
		ShellIconLoader *shellIconLoader);
	void AppendGoMenuItem(MenuView *goMenuView, WORD id, const std::wstring &path,
		ShellIconLoader *shellIconLoader);
	void AppendGoMenuItem(MenuView *goMenuView, WORD id, const PidlAbsolute &pidl,
		ShellIconLoader *shellIconLoader);
	void BuildBookmarksMenu(BrowserWindow *browser, AppServices *appServices,
		IconFetcher *iconFetcher);
	void BuildToolsMenu(const FeatureList *featureList);
	void BuildWindowMenu();
	void BuildHelpMenu();

	void AppendItem(MenuView *menuView, WORD id, UINT textResourceId);
	MenuView *AppendSubMenu(MenuView *menuView, UINT textResourceId);
	std::unique_ptr<IconModel> MaybeCreateIconModel(WORD id) const;
	static std::optional<Icon> GetIconForCommand(WORD id);
	static std::optional<UINT> MaybeGetHelpTextIdForCommand(WORD id);
	MenuItemEnabledCallback MaybeGetEnabledCallback(WORD id) const;
	MenuItemCheckedCallback MaybeGetCheckedCallback(WORD id) const;
	void SendCommandToBrowser(WORD id);

	BrowserWindow *const m_browser;
	const Config *const m_config;
	const ClipboardStore *const m_clipboardStore;
	const FileActionHandler *const m_fileActionHandler;
	const ResourceLoader *const m_resourceLoader;
	CoreInterface *const m_coreInterface;
	std::vector<std::unique_ptr<MenuBase>> m_childMenus;
};
