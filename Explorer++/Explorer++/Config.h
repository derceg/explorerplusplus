// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DefaultColumns.h"
#include "IconResourceLoader.h"
#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ViewModes.h"
#include "ValueWrapper.h"
#include "../Helper/Macros.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/StringHelper.h"

static const int DEFAULT_LISTVIEW_HOVER_TIME = 500;

// These values are used to save/load configuration data and should not be
// changed.
enum class StartupMode
{
	PreviousTabs = 1,
	DefaultFolder = 2
};

enum class InfoTipType
{
	System = 0,
	Custom = 1
};

struct Config
{
	Config() :
		defaultTabDirectoryStatic(GetComputerFolderPath())
	{
		language = LANG_ENGLISH;
		iconTheme = IconTheme::Color;
		startupMode = StartupMode::PreviousTabs;
		defaultTabDirectory = GetComputerFolderPath();
		showStatusBar = TRUE;
		showFolders = TRUE;
		showAddressBar = TRUE;
		showDisplayWindow = TRUE;
		showMainToolbar = TRUE;
		showBookmarksToolbar = FALSE;
		showDrivesToolbar = TRUE;
		showApplicationToolbar = FALSE;
		showTabBarAtBottom = FALSE;
		alwaysOpenNewTab = FALSE;
		openNewTabNextToCurrent = FALSE;
		lockToolbars = TRUE;
		treeViewDelayEnabled = FALSE;
		treeViewAutoExpandSelected = FALSE;
		showTaskbarThumbnails = TRUE;
		useFullRowSelect = FALSE;
		showFilePreviews = TRUE;
		extendTabControl = FALSE;
		allowMultipleInstances = TRUE;
		doubleClickTabClose = TRUE;
		useLargeToolbarIcons.set(FALSE);
		handleZipFiles = FALSE;
		overwriteExistingFilesConfirmation = TRUE;
		checkBoxSelection = FALSE;
		closeMainWindowOnTabClose = TRUE;
		playNavigationSound = TRUE;
		confirmCloseTabs = FALSE;
		synchronizeTreeview = TRUE;
		displayWindowWidth = DEFAULT_DISPLAYWINDOW_WIDTH;
		displayWindowHeight = DEFAULT_DISPLAYWINDOW_HEIGHT;
		displayWindowVertical = FALSE;
		treeViewWidth = DEFAULT_TREEVIEW_WIDTH;

		replaceExplorerMode = DefaultFileManager::ReplaceExplorerMode::None;

		showInfoTips = TRUE;
		infoTipType = InfoTipType::System;

		showFullTitlePath.set(FALSE);
		showUserNameInTitleBar.set(FALSE);
		showPrivilegeLevelInTitleBar.set(FALSE);

		alwaysShowTabBar.set(TRUE);
		forceSameTabWidth.set(FALSE);

		displayWindowSurroundColor = Gdiplus::Color(0, 94, 138);
		displayWindowCentreColor = Gdiplus::Color(255, 255, 255);
		displayWindowTextColor = RGB(0, 0, 0);
		displayWindowFont = CreateFont(-13, 0, 0, 0, FW_MEDIUM, FALSE,
			FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_MODERN,
			_T("Segoe UI"));

		globalFolderSettings.showExtensions = TRUE;
		globalFolderSettings.showFriendlyDates = TRUE;
		globalFolderSettings.showFolderSizes = FALSE;
		globalFolderSettings.disableFolderSizesNetworkRemovable = FALSE;
		globalFolderSettings.hideSystemFiles = FALSE;
		globalFolderSettings.hideLinkExtension = FALSE;
		globalFolderSettings.insertSorted = TRUE;
		globalFolderSettings.showGridlines = TRUE;
		globalFolderSettings.forceSize = FALSE;
		globalFolderSettings.sizeDisplayFormat = SizeDisplayFormat::Bytes;
		globalFolderSettings.oneClickActivate = FALSE;
		globalFolderSettings.oneClickActivateHoverTime = DEFAULT_LISTVIEW_HOVER_TIME;

		globalFolderSettings.folderColumns.realFolderColumns = std::vector<Column_t>(std::begin(REAL_FOLDER_DEFAULT_COLUMNS), std::end(REAL_FOLDER_DEFAULT_COLUMNS));
		globalFolderSettings.folderColumns.myComputerColumns = std::vector<Column_t>(std::begin(MY_COMPUTER_DEFAULT_COLUMNS), std::end(MY_COMPUTER_DEFAULT_COLUMNS));
		globalFolderSettings.folderColumns.controlPanelColumns = std::vector<Column_t>(std::begin(CONTROL_PANEL_DEFAULT_COLUMNS), std::end(CONTROL_PANEL_DEFAULT_COLUMNS));
		globalFolderSettings.folderColumns.recycleBinColumns = std::vector<Column_t>(std::begin(RECYCLE_BIN_DEFAULT_COLUMNS), std::end(RECYCLE_BIN_DEFAULT_COLUMNS));
		globalFolderSettings.folderColumns.printersColumns = std::vector<Column_t>(std::begin(PRINTERS_DEFAULT_COLUMNS), std::end(PRINTERS_DEFAULT_COLUMNS));
		globalFolderSettings.folderColumns.networkConnectionsColumns = std::vector<Column_t>(std::begin(NETWORK_CONNECTIONS_DEFAULT_COLUMNS), std::end(NETWORK_CONNECTIONS_DEFAULT_COLUMNS));
		globalFolderSettings.folderColumns.myNetworkPlacesColumns = std::vector<Column_t>(std::begin(MY_NETWORK_PLACES_DEFAULT_COLUMNS), std::end(MY_NETWORK_PLACES_DEFAULT_COLUMNS));

		defaultFolderSettings.sortMode = SortMode::Name;
		defaultFolderSettings.viewMode = ViewMode::Icons;
		defaultFolderSettings.sortAscending = TRUE;
		defaultFolderSettings.showInGroups = FALSE;
		defaultFolderSettings.showHidden = TRUE;
		defaultFolderSettings.autoArrange = TRUE;
		defaultFolderSettings.applyFilter = FALSE;
		defaultFolderSettings.filterCaseSensitive = FALSE;
	}

	static const UINT DEFAULT_DISPLAYWINDOW_WIDTH = 300;
	static const UINT DEFAULT_DISPLAYWINDOW_HEIGHT = 90;

	static const UINT DEFAULT_TREEVIEW_WIDTH = 208;

	DWORD language;
	IconTheme iconTheme;
	StartupMode startupMode;
	std::wstring defaultTabDirectory;
	const std::wstring defaultTabDirectoryStatic;
	BOOL showStatusBar;
	BOOL showFolders;
	BOOL showAddressBar;
	BOOL showDisplayWindow;
	BOOL showMainToolbar;
	BOOL showBookmarksToolbar;
	BOOL showDrivesToolbar;
	BOOL showApplicationToolbar;
	BOOL showTabBarAtBottom;
	BOOL alwaysOpenNewTab;
	BOOL openNewTabNextToCurrent;
	BOOL lockToolbars;
	BOOL treeViewDelayEnabled;
	BOOL treeViewAutoExpandSelected;
	BOOL showTaskbarThumbnails;
	BOOL useFullRowSelect;
	BOOL showFilePreviews;
	BOOL extendTabControl;
	BOOL allowMultipleInstances;
	BOOL doubleClickTabClose;
	ValueWrapper<BOOL> useLargeToolbarIcons;
	BOOL handleZipFiles;
	BOOL overwriteExistingFilesConfirmation;
	BOOL checkBoxSelection;
	BOOL closeMainWindowOnTabClose;
	BOOL playNavigationSound;
	BOOL confirmCloseTabs;
	BOOL synchronizeTreeview;
	LONG displayWindowWidth;
	LONG displayWindowHeight;
	BOOL displayWindowVertical;
	unsigned int treeViewWidth;

	DefaultFileManager::ReplaceExplorerMode replaceExplorerMode;

	BOOL showInfoTips;
	InfoTipType infoTipType;

	// Main window
	ValueWrapper<BOOL> showFullTitlePath;
	ValueWrapper<BOOL> showUserNameInTitleBar;
	ValueWrapper<BOOL> showPrivilegeLevelInTitleBar;

	// Tabs
	ValueWrapper<BOOL> alwaysShowTabBar;
	ValueWrapper<BOOL> forceSameTabWidth;

	// Display window
	Gdiplus::Color displayWindowCentreColor;
	Gdiplus::Color displayWindowSurroundColor;
	COLORREF displayWindowTextColor;
	HFONT displayWindowFont;

	// These are settings that are shared between all tabs. It's not
	// possible to adjust them on a per-tab basis.
	GlobalFolderSettings globalFolderSettings;

	FolderSettings defaultFolderSettings;

private:

	static std::wstring GetComputerFolderPath()
	{
		// It's assumed here that this won't fail.
		TCHAR computerPath[MAX_PATH];
		GetCsidlDisplayName(CSIDL_DRIVES, computerPath, SIZEOF_ARRAY(computerPath), SHGDN_FORPARSING);
		return computerPath;
	}
};