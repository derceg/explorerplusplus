#pragma once

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
		confirmCloseTabs = FALSE;
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
	BOOL confirmCloseTabs;
};