// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/FolderSettings.h"
#include "ShellBrowser/ViewModes.h"
#include "../Helper/StringHelper.h"

static const int DEFAULT_LISTVIEW_HOVER_TIME = 500;

struct Config
{
	Config()
	{
		showStatusBar = TRUE;
		showFolders = TRUE;
		showAddressBar = TRUE;
		showDisplayWindow = TRUE;
		showMainToolbar = TRUE;
		showBookmarksToolbar = FALSE;
		showDrivesToolbar = TRUE;
		showApplicationToolbar = FALSE;
		alwaysShowTabBar = TRUE;
		showFullTitlePath = FALSE;
		alwaysOpenNewTab = FALSE;
		openNewTabNextToCurrent = FALSE;
		treeViewDelayEnabled = FALSE;
		showUserNameInTitleBar = FALSE;
		showPrivilegeLevelInTitleBar = FALSE;
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

		defaultFolderSettings.sortMode = SortMode::FSM_NAME;
		defaultFolderSettings.viewMode = ViewMode::VM_ICONS;
		defaultFolderSettings.sortAscending = TRUE;
		defaultFolderSettings.showInGroups = FALSE;
		defaultFolderSettings.showHidden = TRUE;
		defaultFolderSettings.autoArrange = TRUE;
		defaultFolderSettings.applyFilter = FALSE;
		defaultFolderSettings.filterCaseSensitive = FALSE;
	}

	BOOL showStatusBar;
	BOOL showFolders;
	BOOL showAddressBar;
	BOOL showDisplayWindow;
	BOOL showMainToolbar;
	BOOL showBookmarksToolbar;
	BOOL showDrivesToolbar;
	BOOL showApplicationToolbar;
	BOOL alwaysShowTabBar;
	BOOL showFullTitlePath;
	BOOL alwaysOpenNewTab;
	BOOL openNewTabNextToCurrent;
	BOOL treeViewDelayEnabled;
	BOOL showUserNameInTitleBar;
	BOOL showPrivilegeLevelInTitleBar;
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

	// These are settings that are shared between all tabs. It's not
	// possible to adjust them on a per-tab basis.
	GlobalFolderSettings globalFolderSettings;

	FolderSettings defaultFolderSettings;
};