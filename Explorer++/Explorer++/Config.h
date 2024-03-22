// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CustomFont.h"
#include "DefaultColumns.h"
#include "IconResourceLoader.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ViewModes.h"
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

enum class ShellChangeNotificationType
{
	Disabled,
	NonFilesystem,
	All
};

// These values are used to save/load configuration data and should not be
// changed.
// clang-format off
BETTER_ENUM(StartupMode, int,
	PreviousTabs = 1,
	DefaultFolder = 2
)
// clang-format on

struct Config
{
	static const UINT DEFAULT_DISPLAYWINDOW_WIDTH = 300;
	static const UINT DEFAULT_DISPLAYWINDOW_HEIGHT = 90;

	static const UINT DEFAULT_TREEVIEW_WIDTH = 208;

	DWORD language = LANG_ENGLISH;
	IconSet iconSet = IconSet::Color;
	ValueWrapper<Theme> theme = Theme::Light;
	StartupMode startupMode = StartupMode::PreviousTabs;
	std::wstring defaultTabDirectory = GetComputerFolderPath();
	const std::wstring defaultTabDirectoryStatic = GetComputerFolderPath();
	bool dualPane = false;
	BOOL showStatusBar = TRUE;
	ValueWrapper<bool> showFolders = true;
	BOOL showAddressBar = TRUE;
	BOOL showDisplayWindow = TRUE;
	BOOL showMainToolbar = TRUE;
	BOOL showBookmarksToolbar = FALSE;
	BOOL showDrivesToolbar = TRUE;
	BOOL showApplicationToolbar = FALSE;
	BOOL alwaysOpenNewTab = FALSE;
	BOOL openNewTabNextToCurrent = FALSE;
	BOOL lockToolbars = TRUE;
	BOOL treeViewDelayEnabled = FALSE;
	BOOL treeViewAutoExpandSelected = FALSE;
	BOOL showTaskbarThumbnails = FALSE;
	ValueWrapper<BOOL> useFullRowSelect = FALSE;
	BOOL showFilePreviews = TRUE;
	BOOL allowMultipleInstances = TRUE;
	BOOL doubleClickTabClose = TRUE;
	ValueWrapper<BOOL> useLargeToolbarIcons = FALSE;
	BOOL handleZipFiles = FALSE;
	BOOL overwriteExistingFilesConfirmation = TRUE;
	ValueWrapper<BOOL> checkBoxSelection = FALSE;
	BOOL closeMainWindowOnTabClose = TRUE;
	BOOL confirmCloseTabs = FALSE;
	ValueWrapper<BOOL> synchronizeTreeview = TRUE;
	LONG displayWindowWidth = DEFAULT_DISPLAYWINDOW_WIDTH;
	LONG displayWindowHeight = DEFAULT_DISPLAYWINDOW_HEIGHT;
	BOOL displayWindowVertical = FALSE;
	int treeViewWidth = DEFAULT_TREEVIEW_WIDTH;
	ShellChangeNotificationType shellChangeNotificationType = ShellChangeNotificationType::All;
	bool goUpOnDoubleClick = true;

	DefaultFileManager::ReplaceExplorerMode replaceExplorerMode =
		DefaultFileManager::ReplaceExplorerMode::None;

	BOOL showInfoTips = TRUE;
	InfoTipType infoTipType = InfoTipType::System;

	ValueWrapper<std::optional<CustomFont>> mainFont;

	// Main window
	ValueWrapper<BOOL> showFullTitlePath = FALSE;
	ValueWrapper<BOOL> showUserNameInTitleBar = FALSE;
	ValueWrapper<BOOL> showPrivilegeLevelInTitleBar = FALSE;

	// Tabs
	ValueWrapper<BOOL> alwaysShowTabBar = TRUE;
	ValueWrapper<BOOL> showTabBarAtBottom = FALSE;
	ValueWrapper<BOOL> extendTabControl = FALSE;
	bool openTabsInForeground = false;

	// Treeview
	bool checkPinnedToNamespaceTreeProperty = false;
	ValueWrapper<bool> showQuickAccessInTreeView = true;

	// Display window
	Gdiplus::Color displayWindowCentreColor = Gdiplus::Color(255, 255, 255);
	Gdiplus::Color displayWindowSurroundColor = Gdiplus::Color(0, 94, 138);
	COLORREF displayWindowTextColor = RGB(0, 0, 0);
	HFONT displayWindowFont = CreateFont(-13, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
		FIXED_PITCH | FF_MODERN, _T("Segoe UI"));

	// These are settings that are shared between all tabs. It's not
	// possible to adjust them on a per-tab basis.
	GlobalFolderSettings globalFolderSettings;

	FolderSettings defaultFolderSettings;

private:
	static std::wstring GetComputerFolderPath()
	{
		// It's assumed here that this won't fail.
		std::wstring computerPath;
		GetCsidlDisplayName(CSIDL_DRIVES, SHGDN_FORPARSING, computerPath);
		return computerPath;
	}
};
