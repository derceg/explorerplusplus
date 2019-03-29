// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ViewModes.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/StringHelper.h"

static const int DEFAULT_LISTVIEW_HOVER_TIME = 500;

enum StartupMode_t
{
	STARTUP_PREVIOUSTABS = 1,
	STARTUP_DEFAULTFOLDER = 2
};

enum InfoTipType_t
{
	INFOTIP_SYSTEM = 0,
	INFOTIP_CUSTOM = 1
};

struct Config
{
	Config()
	{
		startupMode = STARTUP_PREVIOUSTABS;
		showStatusBar = TRUE;
		showFolders = TRUE;
		showAddressBar = TRUE;
		showDisplayWindow = TRUE;
		showMainToolbar = TRUE;
		showBookmarksToolbar = FALSE;
		showDrivesToolbar = TRUE;
		showApplicationToolbar = FALSE;
		alwaysShowTabBar = TRUE;
		showTabBarAtBottom = FALSE;
		showFullTitlePath = FALSE;
		alwaysOpenNewTab = FALSE;
		openNewTabNextToCurrent = FALSE;
		lockToolbars = TRUE;
		treeViewDelayEnabled = FALSE;
		treeViewAutoExpandSelected = FALSE;
		showUserNameInTitleBar = FALSE;
		showPrivilegeLevelInTitleBar = FALSE;
		showTaskbarThumbnails = TRUE;
		useFullRowSelect = FALSE;
		showFilePreviews = TRUE;
		extendTabControl = FALSE;
		allowMultipleInstances = TRUE;
		doubleClickTabClose = TRUE;
		forceSameTabWidth = FALSE;
		useLargeToolbarIcons = FALSE;
		handleZipFiles = FALSE;
		insertSorted = TRUE;
		overwriteExistingFilesConfirmation = TRUE;
		checkBoxSelection = FALSE;
		closeMainWindowOnTabClose = TRUE;
		playNavigationSound = TRUE;
		confirmCloseTabs = FALSE;
		synchronizeTreeview = TRUE;

		replaceExplorerMode = NDefaultFileManager::REPLACEEXPLORER_NONE;

		showInfoTips = TRUE;
		infoTipType = INFOTIP_SYSTEM;

		globalFolderSettings.showExtensions = TRUE;
		globalFolderSettings.showFriendlyDates = TRUE;
		globalFolderSettings.showFolderSizes = FALSE;
		globalFolderSettings.disableFolderSizesNetworkRemovable = FALSE;
		globalFolderSettings.hideSystemFiles = FALSE;
		globalFolderSettings.hideLinkExtension = FALSE;
		globalFolderSettings.showGridlines = TRUE;
		globalFolderSettings.forceSize = FALSE;
		globalFolderSettings.sizeDisplayFormat = SIZE_FORMAT_BYTES;
		globalFolderSettings.oneClickActivate = FALSE;
		globalFolderSettings.oneClickActivateHoverTime = DEFAULT_LISTVIEW_HOVER_TIME;

		defaultFolderSettings.sortMode = SortMode::Name;
		defaultFolderSettings.viewMode = ViewMode::Icons;
		defaultFolderSettings.sortAscending = TRUE;
		defaultFolderSettings.showInGroups = FALSE;
		defaultFolderSettings.showHidden = TRUE;
		defaultFolderSettings.autoArrange = TRUE;
		defaultFolderSettings.applyFilter = FALSE;
		defaultFolderSettings.filterCaseSensitive = FALSE;
	}

	StartupMode_t startupMode;
	BOOL showStatusBar;
	BOOL showFolders;
	BOOL showAddressBar;
	BOOL showDisplayWindow;
	BOOL showMainToolbar;
	BOOL showBookmarksToolbar;
	BOOL showDrivesToolbar;
	BOOL showApplicationToolbar;
	BOOL alwaysShowTabBar;
	BOOL showTabBarAtBottom;
	BOOL showFullTitlePath;
	BOOL alwaysOpenNewTab;
	BOOL openNewTabNextToCurrent;
	BOOL lockToolbars;
	BOOL treeViewDelayEnabled;
	BOOL treeViewAutoExpandSelected;
	BOOL showUserNameInTitleBar;
	BOOL showPrivilegeLevelInTitleBar;
	BOOL showTaskbarThumbnails;
	BOOL useFullRowSelect;
	BOOL showFilePreviews;
	BOOL extendTabControl;
	BOOL allowMultipleInstances;
	BOOL doubleClickTabClose;
	BOOL forceSameTabWidth;
	BOOL useLargeToolbarIcons;
	BOOL handleZipFiles;
	BOOL insertSorted;
	BOOL overwriteExistingFilesConfirmation;
	BOOL checkBoxSelection;
	BOOL closeMainWindowOnTabClose;
	BOOL playNavigationSound;
	BOOL confirmCloseTabs;
	BOOL synchronizeTreeview;

	NDefaultFileManager::ReplaceExplorerModes_t replaceExplorerMode;

	BOOL showInfoTips;
	InfoTipType_t infoTipType;

	// These are settings that are shared between all tabs. It's not
	// possible to adjust them on a per-tab basis.
	GlobalFolderSettings globalFolderSettings;

	FolderSettings defaultFolderSettings;
};