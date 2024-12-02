// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CustomFont.h"
#include "DefaultColumns.h"
#include "DisplayWindowDefaults.h"
#include "FontHelper.h"
#include "IconResourceLoader.h"
#include "LanguageHelper.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ViewModes.h"
#include "ShellChangeNotificationType.h"
#include "Theme.h"
#include "ValueWrapper.h"
#include "../Helper/BetterEnumsWrapper.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"
#include <optional>

// clang-format off
BETTER_ENUM(InfoTipType, int,
	System = 0,
	Custom = 1
)
// clang-format on

// These values are used to save/load configuration data and should not be
// changed.
// clang-format off
BETTER_ENUM(StartupMode, int,
	PreviousTabs = 1,
	DefaultFolder = 2
)
// clang-format on

// Holds application-wide configuration options. Options that are specific to a window or tab should
// instead be stored in their associated class.
struct Config
{
	static const UINT DEFAULT_DISPLAYWINDOW_WIDTH = 300;
	static const UINT DEFAULT_DISPLAYWINDOW_HEIGHT = 90;

	LANGID language = LanguageHelper::DEFAULT_LANGUAGE;
	IconSet iconSet = IconSet::Color;
	ValueWrapper<Theme> theme = +Theme::Light;
	StartupMode startupMode = StartupMode::PreviousTabs;
	std::wstring defaultTabDirectory = GetComputerFolderPath();
	const std::wstring defaultTabDirectoryStatic = GetComputerFolderPath();
	bool dualPane = false;
	bool showStatusBar = true;
	ValueWrapper<bool> showDisplayWindow = true;
	bool alwaysOpenNewTab = false;
	bool openNewTabNextToCurrent = false;
	bool treeViewDelayEnabled = false;
	bool treeViewAutoExpandSelected = false;
	bool showTaskbarThumbnails = false;
	ValueWrapper<bool> useFullRowSelect = false;
	bool showFilePreviews = true;
	bool allowMultipleInstances = true;
	bool doubleClickTabClose = true;
	ValueWrapper<bool> useLargeToolbarIcons = false;
	bool handleZipFiles = false;
	bool overwriteExistingFilesConfirmation = true;
	ValueWrapper<bool> checkBoxSelection = false;
	bool closeMainWindowOnTabClose = true;
	bool confirmCloseTabs = false;
	ValueWrapper<bool> synchronizeTreeview = true;
	int displayWindowWidth = DEFAULT_DISPLAYWINDOW_WIDTH;
	int displayWindowHeight = DEFAULT_DISPLAYWINDOW_HEIGHT;
	bool displayWindowVertical = false;
	ShellChangeNotificationType shellChangeNotificationType = ShellChangeNotificationType::All;
	bool goUpOnDoubleClick = true;

	DefaultFileManager::ReplaceExplorerMode replaceExplorerMode =
		DefaultFileManager::ReplaceExplorerMode::None;

	bool showInfoTips = true;
	InfoTipType infoTipType = InfoTipType::System;

	ValueWrapper<std::optional<CustomFont>> mainFont;

	// Main window
	ValueWrapper<bool> showFullTitlePath = false;
	ValueWrapper<bool> showUserNameInTitleBar = false;
	ValueWrapper<bool> showPrivilegeLevelInTitleBar = false;

	// Toolbar display settings
	ValueWrapper<bool> showFolders = true;
	ValueWrapper<bool> showAddressBar = true;
	ValueWrapper<bool> showMainToolbar = true;
	ValueWrapper<bool> showBookmarksToolbar = false;
	ValueWrapper<bool> showDrivesToolbar = true;
	ValueWrapper<bool> showApplicationToolbar = false;
	ValueWrapper<bool> lockToolbars = true;

	// Tabs
	ValueWrapper<bool> alwaysShowTabBar = true;
	ValueWrapper<bool> showTabBarAtBottom = false;
	ValueWrapper<bool> extendTabControl = false;
	bool openTabsInForeground = false;

	// Treeview
	bool checkPinnedToNamespaceTreeProperty = false;
	ValueWrapper<bool> showQuickAccessInTreeView = true;

	// Display window
	ValueWrapper<COLORREF> displayWindowCentreColor = DisplayWindowDefaults::CENTRE_COLOR;
	ValueWrapper<COLORREF> displayWindowSurroundColor = DisplayWindowDefaults::SURROUND_COLOR;
	ValueWrapper<COLORREF> displayWindowTextColor = DisplayWindowDefaults::TEXT_COLOR;
	ValueWrapper<LOGFONT> displayWindowFont = DisplayWindowDefaults::FONT;

	// These are settings that are shared between all tabs. It's not
	// possible to adjust them on a per-tab basis.
	GlobalFolderSettings globalFolderSettings;

	FolderSettings defaultFolderSettings;

	// This is only used in tests.
	bool operator==(const Config &) const = default;

private:
	static std::wstring GetComputerFolderPath()
	{
		// It's assumed here that this won't fail.
		std::wstring computerPath;
		GetCsidlDisplayName(CSIDL_DRIVES, SHGDN_FORPARSING, computerPath);
		return computerPath;
	}
};
