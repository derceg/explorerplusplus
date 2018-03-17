/******************************************************************
 *
 * Project: Explorer++
 * File: Settings.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Saves and loads all main program settings.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "DefaultColumns.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"
#include "../DisplayWindow/DisplayWindow.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/Macros.h"


namespace
{
	const TCHAR REG_BOOKMARKS_KEY[] = _T("Software\\Explorer++\\Bookmarks");
	const TCHAR REG_TABS_KEY[] = _T("Software\\Explorer++\\Tabs");
	const TCHAR REG_TOOLBARS_KEY[] = _T("Software\\Explorer++\\Toolbars");
	const TCHAR REG_COLUMNS_KEY[] = _T("Software\\Explorer++\\DefaultColumns");
	const TCHAR REG_APPLICATIONS_KEY[] = _T("Software\\Explorer++\\ApplicationToolbar");
}

#define DEFAULT_DISPLAYWINDOW_CENTRE_COLOR		Gdiplus::Color(255,255,255)
#define DEFAULT_DISPLAYWINDOW_SURROUND_COLOR	Gdiplus::Color(0,94,138)

BOOL LoadWindowPositionFromRegistry(WINDOWPLACEMENT *pwndpl)
{
	HKEY hSettingsKey;
	BOOL bRes = FALSE;

	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,NExplorerplusplus::REG_SETTINGS_KEY,0,
		KEY_READ,&hSettingsKey);

	if(lRes == ERROR_SUCCESS)
	{
		DWORD dwSize = sizeof(WINDOWPLACEMENT);

		RegQueryValueEx(hSettingsKey,_T("Position"),NULL,NULL,
			(LPBYTE)pwndpl,&dwSize);

		if(dwSize == sizeof(WINDOWPLACEMENT) &&
			pwndpl->length == sizeof(WINDOWPLACEMENT))
		{
			bRes = TRUE;
		}

		RegCloseKey(hSettingsKey);
	}

	return bRes;
}

BOOL LoadAllowMultipleInstancesFromRegistry(void)
{
	BOOL bAllowMultipleInstances = TRUE;

	HKEY hSettingsKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,NExplorerplusplus::REG_SETTINGS_KEY,0,KEY_READ,&hSettingsKey);

	if(lRes == ERROR_SUCCESS)
	{
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("AllowMultipleInstances"),
			(LPDWORD)&bAllowMultipleInstances);

		RegCloseKey(hSettingsKey);
	}

	return bAllowMultipleInstances;
}

LONG Explorerplusplus::SaveSettings(void)
{
	HKEY			hSettingsKey;
	DWORD			Disposition;
	LONG			ReturnValue;
	TBSAVEPARAMS	tbSave;

	/* Open/Create the main key that is used to store data. */
	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,NExplorerplusplus::REG_SETTINGS_KEY,0,NULL,
	REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hSettingsKey,&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		WINDOWPLACEMENT wndpl;

		wndpl.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(m_hContainer,&wndpl);

		/* Window position. */
		RegSetValueEx(hSettingsKey,_T("Position"),0,REG_BINARY,
			(LPBYTE)&wndpl,sizeof(wndpl));

		/* User settings. */
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("LastSelectedTab"),m_iLastSelectedTab);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowExtensions"),m_bShowExtensionsGlobal);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowStatusBar"),m_bShowStatusBar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowFolders"),m_bShowFolders);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowAddressBar"),m_bShowAddressBar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowToolbar"),m_bShowMainToolbar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowBookmarksToolbar"),m_bShowBookmarksToolbar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowDrivesToolbar"),m_bShowDrivesToolbar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowApplicationToolbar"),m_bShowApplicationToolbar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowFullTitlePath"),m_bShowFullTitlePath);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("AlwaysOpenNewTab"),m_bAlwaysOpenNewTab);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("TreeViewWidth"),m_TreeViewWidth);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowFriendlyDates"),m_bShowFriendlyDatesGlobal);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowDisplayWindow"),m_bShowDisplayWindow);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowFolderSizes"),m_bShowFolderSizes);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("DisableFolderSizesNetworkRemovable"),m_bDisableFolderSizesNetworkRemovable);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("StartupMode"),m_StartupMode);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("NextToCurrent"),m_bOpenNewTabNextToCurrent);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ConfirmCloseTabs"),m_bConfirmCloseTabs);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowInfoTips"),m_bShowInfoTips);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("InfoTipType"),m_InfoTipType);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("TreeViewDelayEnabled"),m_bTreeViewDelayEnabled);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("LockToolbars"),m_bLockToolbars);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ExtendTabControl"),m_bExtendTabControl);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("UseFullRowSelect"),m_bUseFullRowSelect);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowFilePreviews"),m_bShowFilePreviews);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ReplaceExplorerMode"),m_ReplaceExplorerMode);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowUserNameTitleBar"),m_bShowUserNameInTitleBar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("AllowMultipleInstances"),m_bAllowMultipleInstances);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("OneClickActivate"),m_bOneClickActivate);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("OneClickActivateHoverTime"),m_OneClickActivateHoverTime);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ForceSameTabWidth"),m_bForceSameTabWidth);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("DoubleClickTabClose"),m_bDoubleClickTabClose);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("HandleZipFiles"),m_bHandleZipFiles);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("InsertSorted"),m_bInsertSorted);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowPrivilegeLevelInTitleBar"),m_bShowPrivilegeLevelInTitleBar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("AlwaysShowTabBar"),m_bAlwaysShowTabBar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("CheckBoxSelection"),m_bCheckBoxSelection);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ForceSize"),m_bForceSize);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("SizeDisplayFormat"),m_SizeDisplayFormat);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("CloseMainWindowOnTabClose"),m_bCloseMainWindowOnTabClose);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowTabBarAtBottom"),m_bShowTabBarAtBottom);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("OverwriteExistingFilesConfirmation"),m_bOverwriteExistingFilesConfirmation);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("LargeToolbarIcons"),m_bLargeToolbarIcons);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("PlayNavigationSound"),m_bPlayNavigationSound);

		NRegistrySettings::SaveStringToRegistry(hSettingsKey,_T("NewTabDirectory"),m_DefaultTabDirectory);

		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("Language"),m_Language);

		/* Global settings. */
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowHiddenGlobal"),m_bShowHiddenGlobal);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ViewModeGlobal"),m_ViewModeGlobal);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowGridlinesGlobal"),m_bShowGridlinesGlobal);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowInGroupsGlobal"),m_bShowInGroupsGlobal);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("AutoArrangeGlobal"),m_bAutoArrangeGlobal);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("SortAscendingGlobal"),m_bSortAscendingGlobal);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("HideSystemFilesGlobal"),m_bHideSystemFilesGlobal);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("HideLinkExtensionGlobal"),m_bHideLinkExtensionGlobal);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowTaskbarThumbnails"),m_bShowTaskbarThumbnails);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("SynchronizeTreeview"),m_bSynchronizeTreeview);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("TVAutoExpandSelected"),m_bTVAutoExpandSelected);

		/* Display window settings. */
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("DisplayWindowHeight"),m_DisplayWindowHeight);

		COLORREF CentreColor;
		COLORREF SurroundColor;
		COLORREF TextColor;
		LOGFONT LogFont;
		HFONT hFont;

		CentreColor = (COLORREF)SendMessage(m_hDisplayWindow,DWM_GETCENTRECOLOR,0,0);
		SurroundColor = (COLORREF)SendMessage(m_hDisplayWindow,DWM_GETSURROUNDCOLOR,0,0);
		TextColor = (COLORREF)SendMessage(m_hDisplayWindow,DWM_GETTEXTCOLOR,0,0);
		SendMessage(m_hDisplayWindow,DWM_GETFONT,(WPARAM)&hFont,0);

		RegSetValueEx(hSettingsKey,_T("DisplayCentreColor"),0,REG_BINARY,
			(LPBYTE)&CentreColor,sizeof(CentreColor));

		RegSetValueEx(hSettingsKey,_T("DisplaySurroundColor"),0,REG_BINARY,
			(LPBYTE)&SurroundColor,sizeof(SurroundColor));

		RegSetValueEx(hSettingsKey,_T("DisplayTextColor"),0,REG_BINARY,
			(LPBYTE)&TextColor,sizeof(TextColor));

		GetObject(hFont,sizeof(LOGFONT),(LPVOID)&LogFont);

		RegSetValueEx(hSettingsKey,_T("DisplayFont"),0,REG_BINARY,
			(LPBYTE)&LogFont,sizeof(LOGFONT));

		tbSave.hkr			= HKEY_CURRENT_USER;
		tbSave.pszSubKey	= NExplorerplusplus::REG_SETTINGS_KEY;
		tbSave.pszValueName	= _T("ToolbarState");

		SendMessage(m_hMainToolbar,TB_SAVERESTORE,TRUE,(LPARAM)&tbSave);

		RegCloseKey(hSettingsKey);
	}

	return ReturnValue;
}

LONG Explorerplusplus::LoadSettings()
{
	HKEY			hSettingsKey;
	LONG			ReturnValue;
	LONG			CentreColorStatus = TRUE;
	LONG			SurroundColorStatus = TRUE;
	LONG			TextColorStatus = TRUE;
	LONG			FontStatus = TRUE;
	LONG			lStatus;

	/* Open/Create the main key that is used to store data. */
	ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER, NExplorerplusplus::REG_SETTINGS_KEY,0,KEY_READ,&hSettingsKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		/* User settings. */
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("LastSelectedTab"),(LPDWORD)&m_iLastSelectedTab);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowExtensions"),(LPDWORD)&m_bShowExtensionsGlobal);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowStatusBar"),(LPDWORD)&m_bShowStatusBar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowFolders"),(LPDWORD)&m_bShowFolders);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowAddressBar"),(LPDWORD)&m_bShowAddressBar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowToolbar"),(LPDWORD)&m_bShowMainToolbar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowBookmarksToolbar"),(LPDWORD)&m_bShowBookmarksToolbar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowDrivesToolbar"),(LPDWORD)&m_bShowDrivesToolbar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowApplicationToolbar"),(LPDWORD)&m_bShowApplicationToolbar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowFullTitlePath"),(LPDWORD)&m_bShowFullTitlePath);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("AlwaysOpenNewTab"),(LPDWORD)&m_bAlwaysOpenNewTab);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("TreeViewWidth"),(LPDWORD)&m_TreeViewWidth);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowFriendlyDates"),(LPDWORD)&m_bShowFriendlyDatesGlobal);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowDisplayWindow"),(LPDWORD)&m_bShowDisplayWindow);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowFolderSizes"),(LPDWORD)&m_bShowFolderSizes);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("DisableFolderSizesNetworkRemovable"),(LPDWORD)&m_bDisableFolderSizesNetworkRemovable);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("StartupMode"),(LPDWORD)&m_StartupMode);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("NextToCurrent"),(LPDWORD)&m_bOpenNewTabNextToCurrent);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ConfirmCloseTabs"),(LPDWORD)&m_bConfirmCloseTabs);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowInfoTips"),(LPDWORD)&m_bShowInfoTips);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("InfoTipType"),(LPDWORD)&m_InfoTipType);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("TreeViewDelayEnabled"),(LPDWORD)&m_bTreeViewDelayEnabled);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("LockToolbars"),(LPDWORD)&m_bLockToolbars);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ExtendTabControl"),(LPDWORD)&m_bExtendTabControl);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("UseFullRowSelect"),(LPDWORD)&m_bUseFullRowSelect);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowFilePreviews"),(LPDWORD)&m_bShowFilePreviews);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ReplaceExplorerMode"),(LPDWORD)&m_ReplaceExplorerMode);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowUserNameTitleBar"),(LPDWORD)&m_bShowUserNameInTitleBar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("AllowMultipleInstances"),(LPDWORD)&m_bAllowMultipleInstances);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("OneClickActivate"),(LPDWORD)&m_bOneClickActivate);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("OneClickActivateHoverTime"),(LPDWORD)&m_OneClickActivateHoverTime);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ForceSameTabWidth"),(LPDWORD)&m_bForceSameTabWidth);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("DoubleClickTabClose"),(LPDWORD)&m_bDoubleClickTabClose);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("HandleZipFiles"),(LPDWORD)&m_bHandleZipFiles);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("InsertSorted"),(LPDWORD)&m_bInsertSorted);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowPrivilegeLevelInTitleBar"),(LPDWORD)&m_bShowPrivilegeLevelInTitleBar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("AlwaysShowTabBar"),(LPDWORD)&m_bAlwaysShowTabBar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("CheckBoxSelection"),(LPDWORD)&m_bCheckBoxSelection);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ForceSize"),(LPDWORD)&m_bForceSize);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("SizeDisplayFormat"),(LPDWORD)&m_SizeDisplayFormat);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("CloseMainWindowOnTabClose"),(LPDWORD)&m_bCloseMainWindowOnTabClose);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowTabBarAtBottom"),(LPDWORD)&m_bShowTabBarAtBottom);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowTaskbarThumbnails"),(LPDWORD)&m_bShowTaskbarThumbnails);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("SynchronizeTreeview"),(LPDWORD)&m_bSynchronizeTreeview);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("TVAutoExpandSelected"),(LPDWORD)&m_bTVAutoExpandSelected);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("OverwriteExistingFilesConfirmation"),(LPDWORD)&m_bOverwriteExistingFilesConfirmation);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("LargeToolbarIcons"),(LPDWORD)&m_bLargeToolbarIcons);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("PlayNavigationSound"),(LPDWORD)&m_bPlayNavigationSound);

		NRegistrySettings::ReadStringFromRegistry(hSettingsKey,_T("NewTabDirectory"),m_DefaultTabDirectory,SIZEOF_ARRAY(m_DefaultTabDirectory));

		lStatus = NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("Language"),(LPDWORD)&m_Language);

		if(lStatus == ERROR_SUCCESS)
			m_bLanguageLoaded = TRUE;

		/* Global settings. */
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowHiddenGlobal"),(LPDWORD)&m_bShowHiddenGlobal);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ViewModeGlobal"),(LPDWORD)&m_ViewModeGlobal);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowGridlinesGlobal"),(LPDWORD)&m_bShowGridlinesGlobal);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowInGroupsGlobal"),(LPDWORD)&m_bShowInGroupsGlobal);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("AutoArrangeGlobal"),(LPDWORD)&m_bAutoArrangeGlobal);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("SortAscendingGlobal"),(LPDWORD)&m_bSortAscendingGlobal);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("HideSystemFilesGlobal"),(LPDWORD)&m_bHideSystemFilesGlobal);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("HideLinkExtensionGlobal"),(LPDWORD)&m_bHideLinkExtensionGlobal);

		/* Display window settings. */
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("DisplayWindowHeight"),(LPDWORD)&m_DisplayWindowHeight);

		COLORREF CentreColor;
		COLORREF SurroundColor;
		COLORREF TextColor;
		HFONT hFont;
		LOGFONT LogFont;
		DWORD dwType;
		DWORD dwSize;

		dwType = REG_BINARY;
		dwSize = sizeof(SurroundColor);

		SurroundColorStatus = RegQueryValueEx(hSettingsKey,_T("DisplaySurroundColor"),0,&dwType,(LPBYTE)&SurroundColor,
			&dwSize);

		dwType = REG_BINARY;
		dwSize = sizeof(CentreColor);

		CentreColorStatus = RegQueryValueEx(hSettingsKey,_T("DisplayCentreColor"),0,&dwType,(LPBYTE)&CentreColor,
			&dwSize);

		dwType = REG_BINARY;
		dwSize = sizeof(TextColor);

		TextColorStatus = RegQueryValueEx(hSettingsKey,_T("DisplayTextColor"),0,&dwType,(LPBYTE)&TextColor,
			&dwSize);

		dwType = REG_BINARY;
		dwSize = sizeof(LOGFONT);

		FontStatus = RegQueryValueEx(hSettingsKey,_T("DisplayFont"),0,&dwType,(LPBYTE)&LogFont,
			&dwSize);
		hFont = CreateFontIndirect(&LogFont);

		m_DisplayWindowCentreColor.SetFromCOLORREF(CentreColor);
		m_DisplayWindowSurroundColor.SetFromCOLORREF(SurroundColor);
		m_DisplayWindowTextColor = TextColor;
		m_DisplayWindowFont = hFont;

		m_bAttemptToolbarRestore = TRUE;

		RegCloseKey(hSettingsKey);
	}

	SetInitialToolbarButtons();

	if(SurroundColorStatus != ERROR_SUCCESS)
		m_DisplayWindowSurroundColor	= DEFAULT_DISPLAYWINDOW_SURROUND_COLOR;

	if(CentreColorStatus != ERROR_SUCCESS)
		m_DisplayWindowCentreColor	= DEFAULT_DISPLAYWINDOW_CENTRE_COLOR;

	if(TextColorStatus != ERROR_SUCCESS)
		m_DisplayWindowTextColor		= RGB(0,0,0);

	if(FontStatus != ERROR_SUCCESS)
		m_DisplayWindowFont	= CreateFont(-13,0,0,0,FW_MEDIUM,FALSE,
			FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,PROOF_QUALITY,FIXED_PITCH|FF_MODERN,
			_T("Segoe UI"));

	return ReturnValue;
}

void DeleteKey(HKEY hKey)
{
	HKEY	hChildKey;
	TCHAR	lpName[512];
	DWORD	dwName;
	DWORD	nSubKeys;
	DWORD	nChildSubKeys;
	DWORD	Disposition;
	LONG	ReturnValue;
	int		i = 0;

	/* Enumerate all the previous bookmarks keys and
	delete them. */
	if(RegQueryInfoKey(hKey,NULL,NULL,NULL,&nSubKeys,NULL,NULL,
		NULL,NULL,NULL,NULL,NULL) == ERROR_SUCCESS)
	{
		for(i = nSubKeys - 1;i >= 0;i--)
		{
			dwName = SIZEOF_ARRAY(lpName);

			if(RegEnumKeyEx(hKey,i,lpName,&dwName,
				0,NULL,NULL,NULL) == ERROR_SUCCESS)
			{
				ReturnValue = RegCreateKeyEx(hKey,lpName,0,
					NULL,REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_ENUMERATE_SUB_KEYS |
					KEY_QUERY_VALUE | DELETE,NULL,&hChildKey,&Disposition);

				if(ReturnValue == ERROR_SUCCESS)
				{
					RegQueryInfoKey(hChildKey,NULL,NULL,NULL,&nChildSubKeys,
						NULL,NULL,NULL,NULL,NULL,NULL,NULL);

					/* If this key contains subkeys, it cannot just
					be deleted. It must have each of it's subkeys
					deleted individually. */
					if(nChildSubKeys != 0)
					{
						DeleteKey(hChildKey);
					}

					RegCloseKey(hChildKey);

					RegDeleteKey(hKey,lpName);
				}
			}
		}
	}
}

void Explorerplusplus::SaveBookmarksToRegistry(void)
{
	SHDeleteKey(HKEY_CURRENT_USER,REG_BOOKMARKS_KEY);
	m_bfAllBookmarks->SerializeToRegistry(REG_BOOKMARKS_KEY);
}

void Explorerplusplus::LoadBookmarksFromRegistry(void)
{
	HKEY		hBookmarksKey;
	LONG		ReturnValue;

	ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_BOOKMARKS_KEY,
		0,KEY_READ,&hBookmarksKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		/* TODO: Load bookmarks. */

		RegCloseKey(hBookmarksKey);
	}
}

void Explorerplusplus::SaveTabSettingsToRegistry(void)
{
	HKEY	hKey;
	HKEY	hTabKey;
	HKEY	hColumnsKey;
	TCITEM	tcItem;
	TCHAR	szItemKey[128];
	LPITEMIDLIST	pidlDirectory = NULL;
	UINT	ViewMode;
	UINT	SortMode;
	DWORD	Disposition;
	LONG	ReturnValue;
	int		iNumTabs;
	int		i = 0;

	/* First, delete all current tab keys. If these keys
	are not deleted beforehand, then they may be opened
	again on the next program run, even when they were
	closed. */
	SHDeleteKey(HKEY_CURRENT_USER,REG_TABS_KEY);

	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_TABS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		iNumTabs = TabCtrl_GetItemCount(m_hTabCtrl);

		tcItem.mask = TCIF_PARAM;

		for(i = 0;i < iNumTabs;i++)
		{
			TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
				_T("%d"),i);

			ReturnValue = RegCreateKeyEx(hKey,szItemKey,0,NULL,
				REG_OPTION_NON_VOLATILE,KEY_WRITE,
				NULL,&hTabKey,&Disposition);

			if(ReturnValue == ERROR_SUCCESS)
			{
				pidlDirectory = m_pShellBrowser[(int)tcItem.lParam]->QueryCurrentDirectoryIdl();
				RegSetValueEx(hTabKey,_T("Directory"),0,REG_BINARY,
					(LPBYTE)pidlDirectory,ILGetSize(pidlDirectory));
				CoTaskMemFree((LPVOID)pidlDirectory);

				ViewMode = m_pShellBrowser[(int) tcItem.lParam]->GetCurrentViewMode();

				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ViewMode"),ViewMode);

				SortMode = m_pShellBrowser[(int) tcItem.lParam]->GetSortMode();
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("SortMode"),SortMode);

				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("SortAscending"),m_pShellBrowser[(int)tcItem.lParam]->GetSortAscending());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ShowInGroups"),m_pShellBrowser[(int)tcItem.lParam]->IsGroupViewEnabled());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ApplyFilter"),m_pShellBrowser[(int)tcItem.lParam]->GetFilterStatus());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("FilterCaseSensitive"),m_pShellBrowser[(int)tcItem.lParam]->GetFilterCaseSensitive());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ShowHidden"),m_pShellBrowser[(int)tcItem.lParam]->GetShowHidden());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("AutoArrange"),m_pShellBrowser[(int)tcItem.lParam]->GetAutoArrange());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ShowGridlines"),m_pShellBrowser[(int)tcItem.lParam]->QueryGridlinesActive());

				TCHAR szFilter[512];

				m_pShellBrowser[(int)tcItem.lParam]->GetFilter(szFilter,SIZEOF_ARRAY(szFilter));
				NRegistrySettings::SaveStringToRegistry(hTabKey,_T("Filter"),szFilter);

				/* Now save the tabs columns. */
				ReturnValue = RegCreateKeyEx(hTabKey,_T("Columns"),
					0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hColumnsKey,
					&Disposition);

				if(ReturnValue == ERROR_SUCCESS)
				{
					ColumnExport_t cie;

					m_pShellBrowser[(int)tcItem.lParam]->ExportAllColumns(&cie);

					SaveColumnToRegistry(hColumnsKey,_T("ControlPanelColumns"),&cie.ControlPanelColumnList);
					SaveColumnToRegistry(hColumnsKey,_T("MyComputerColumns"),&cie.MyComputerColumnList);
					SaveColumnToRegistry(hColumnsKey,_T("RealFolderColumns"),&cie.RealFolderColumnList);
					SaveColumnToRegistry(hColumnsKey,_T("RecycleBinColumns"),&cie.RecycleBinColumnList);
					SaveColumnToRegistry(hColumnsKey,_T("PrinterColumns"),&cie.PrintersColumnList);
					SaveColumnToRegistry(hColumnsKey,_T("NetworkColumns"),&cie.NetworkConnectionsColumnList);
					SaveColumnToRegistry(hColumnsKey,_T("NetworkPlacesColumns"),&cie.MyNetworkPlacesColumnList);

					/* Now save column widths. In the future, these keys may be merged with
					the column keys above. */
					SaveColumnWidthsToRegistry(hColumnsKey,_T("ControlPanelColumnWidths"),&cie.ControlPanelColumnList);
					SaveColumnWidthsToRegistry(hColumnsKey,_T("MyComputerColumnWidths"),&cie.MyComputerColumnList);
					SaveColumnWidthsToRegistry(hColumnsKey,_T("RealFolderColumnWidths"),&cie.RealFolderColumnList);
					SaveColumnWidthsToRegistry(hColumnsKey,_T("RecycleBinColumnWidths"),&cie.RecycleBinColumnList);
					SaveColumnWidthsToRegistry(hColumnsKey,_T("PrinterColumnWidths"),&cie.PrintersColumnList);
					SaveColumnWidthsToRegistry(hColumnsKey,_T("NetworkColumnWidths"),&cie.NetworkConnectionsColumnList);
					SaveColumnWidthsToRegistry(hColumnsKey,_T("NetworkPlacesColumnWidths"),&cie.MyNetworkPlacesColumnList);

					RegCloseKey(hColumnsKey);
				}

				/* High-level settings. */
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("Locked"),m_TabInfo.at((int)tcItem.lParam).bLocked);
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("AddressLocked"),m_TabInfo.at((int)tcItem.lParam).bAddressLocked);
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("UseCustomName"),m_TabInfo.at((int)tcItem.lParam).bUseCustomName);

				if(m_TabInfo.at((int)tcItem.lParam).bUseCustomName)
					NRegistrySettings::SaveStringToRegistry(hTabKey,_T("CustomName"),m_TabInfo.at((int)tcItem.lParam).szName);
				else
					NRegistrySettings::SaveStringToRegistry(hTabKey,_T("CustomName"),EMPTY_STRING);

				RegCloseKey(hTabKey);
			}
		}

		RegCloseKey(hKey);
	}
}

void UpdateColumnWidths(std::list<Column_t> *pColumnList,std::list<Column_t> *pColumnWidthList)
{
	std::list<Column_t>::iterator itr1;
	std::list<Column_t>::iterator itr2;

	for(itr1 = pColumnWidthList->begin();itr1 != pColumnWidthList->end();itr1++)
	{
		for(itr2 = pColumnList->begin();itr2 != pColumnList->end();itr2++)
		{
			if(itr2->id == itr1->id)
			{
				itr2->iWidth = itr1->iWidth;
				break;
			}
		}
	}
}

int Explorerplusplus::LoadTabSettingsFromRegistry(void)
{
	HKEY				hKey;
	HKEY				hTabKey;
	HKEY				hColumnsKey;
	InitialSettings_t	Settings;
	TabInfo_t			TabInfo;
	TCHAR				szItemKey[128];
	LPITEMIDLIST		pidlDirectory = NULL;
	LONG				ReturnValue;
	DWORD				cbData;
	DWORD				Type;
	HRESULT				hr;
	int					nTabsCreated = 0;
	int					i = 0;

	ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_TABS_KEY,0,KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
			_T("%d"),i);

		ReturnValue = RegOpenKeyEx(hKey,szItemKey,0,KEY_READ,&hTabKey);

		while(ReturnValue == ERROR_SUCCESS)
		{
			if(RegQueryValueEx(hTabKey,_T("Directory"),0,NULL,NULL,&cbData)
				== ERROR_SUCCESS)
			{
				pidlDirectory = (LPITEMIDLIST)CoTaskMemAlloc(cbData);

				RegQueryValueEx(hTabKey,_T("Directory"),0,&Type,(LPBYTE)pidlDirectory,&cbData);
			}

			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("ViewMode"),(LPDWORD)&Settings.ViewMode);

			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("SortMode"),(LPDWORD)&Settings.SortMode);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("SortAscending"),(LPDWORD)&Settings.bSortAscending);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("ShowInGroups"),(LPDWORD)&Settings.bShowInGroups);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("ApplyFilter"),(LPDWORD)&Settings.bApplyFilter);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("FilterCaseSensitive"),(LPDWORD)&Settings.bFilterCaseSensitive);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("ShowHidden"),(LPDWORD)&Settings.bShowHidden);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("AutoArrange"),(LPDWORD)&Settings.bAutoArrange);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("ShowGridlines"),(LPDWORD)&Settings.bGridlinesActive);
			NRegistrySettings::ReadStringFromRegistry(hTabKey,_T("Filter"),Settings.szFilter,SIZEOF_ARRAY(Settings.szFilter));

			Settings.bShowFolderSizes = m_bShowFolderSizes;
			Settings.bDisableFolderSizesNetworkRemovable = m_bDisableFolderSizesNetworkRemovable;

			std::list<Column_t>	RealFolderColumnList;
			std::list<Column_t>	MyComputerColumnList;
			std::list<Column_t>	ControlPanelColumnList;
			std::list<Column_t>	RecycleBinColumnList;
			std::list<Column_t>	PrintersColumnList;
			std::list<Column_t>	NetworkConnectionsColumnList;
			std::list<Column_t>	MyNetworkPlacesColumnList;

			/* Now load this tabs columns. */
			ReturnValue = RegOpenKeyEx(hTabKey,_T("Columns"),0,KEY_READ,&hColumnsKey);

			if(ReturnValue == ERROR_SUCCESS)
			{
				LoadColumnFromRegistry(hColumnsKey,_T("ControlPanelColumns"),&ControlPanelColumnList);
				LoadColumnFromRegistry(hColumnsKey,_T("MyComputerColumns"),&MyComputerColumnList);
				LoadColumnFromRegistry(hColumnsKey,_T("RealFolderColumns"),&RealFolderColumnList);
				LoadColumnFromRegistry(hColumnsKey,_T("RecycleBinColumns"),&RecycleBinColumnList);
				LoadColumnFromRegistry(hColumnsKey,_T("PrinterColumns"),&PrintersColumnList);
				LoadColumnFromRegistry(hColumnsKey,_T("NetworkColumns"),&NetworkConnectionsColumnList);
				LoadColumnFromRegistry(hColumnsKey,_T("NetworkPlacesColumns"),&MyNetworkPlacesColumnList);

				std::list<Column_t>	RealFolderColumnListTemp;
				std::list<Column_t>	MyComputerColumnListTemp;
				std::list<Column_t>	ControlPanelColumnListTemp;
				std::list<Column_t>	RecycleBinColumnListTemp;
				std::list<Column_t>	PrintersColumnListTemp;
				std::list<Column_t>	NetworkConnectionsColumnListTemp;
				std::list<Column_t>	MyNetworkPlacesColumnListTemp;

				LoadColumnWidthsFromRegistry(hColumnsKey,_T("ControlPanelColumnWidths"),&ControlPanelColumnListTemp);
				LoadColumnWidthsFromRegistry(hColumnsKey,_T("MyComputerColumnWidths"),&MyComputerColumnListTemp);
				LoadColumnWidthsFromRegistry(hColumnsKey,_T("RealFolderColumnWidths"),&RealFolderColumnListTemp);
				LoadColumnWidthsFromRegistry(hColumnsKey,_T("RecycleBinColumnWidths"),&RecycleBinColumnListTemp);
				LoadColumnWidthsFromRegistry(hColumnsKey,_T("PrinterColumnWidths"),&PrintersColumnListTemp);
				LoadColumnWidthsFromRegistry(hColumnsKey,_T("NetworkColumnWidths"),&NetworkConnectionsColumnListTemp);
				LoadColumnWidthsFromRegistry(hColumnsKey,_T("NetworkPlacesColumnWidths"),&MyNetworkPlacesColumnListTemp);

				UpdateColumnWidths(&ControlPanelColumnList,&ControlPanelColumnListTemp);
				UpdateColumnWidths(&MyComputerColumnList,&MyComputerColumnListTemp);
				UpdateColumnWidths(&RealFolderColumnList,&RealFolderColumnListTemp);
				UpdateColumnWidths(&RecycleBinColumnList,&RecycleBinColumnListTemp);
				UpdateColumnWidths(&PrintersColumnList,&PrintersColumnListTemp);
				UpdateColumnWidths(&NetworkConnectionsColumnList,&NetworkConnectionsColumnListTemp);
				UpdateColumnWidths(&MyNetworkPlacesColumnList,&MyNetworkPlacesColumnListTemp);

				RegCloseKey(hColumnsKey);
			}

			ValidateSingleColumnSet(VALIDATE_REALFOLDER_COLUMNS,&RealFolderColumnList);
			ValidateSingleColumnSet(VALIDATE_CONTROLPANEL_COLUMNS,&ControlPanelColumnList);
			ValidateSingleColumnSet(VALIDATE_MYCOMPUTER_COLUMNS,&MyComputerColumnList);
			ValidateSingleColumnSet(VALIDATE_RECYCLEBIN_COLUMNS,&RecycleBinColumnList);
			ValidateSingleColumnSet(VALIDATE_PRINTERS_COLUMNS,&PrintersColumnList);
			ValidateSingleColumnSet(VALIDATE_NETWORKCONNECTIONS_COLUMNS,&NetworkConnectionsColumnList);
			ValidateSingleColumnSet(VALIDATE_MYNETWORKPLACES_COLUMNS,&MyNetworkPlacesColumnList);

			Settings.pControlPanelColumnList		= &ControlPanelColumnList;
			Settings.pMyComputerColumnList			= &MyComputerColumnList;
			Settings.pMyNetworkPlacesColumnList		= &MyNetworkPlacesColumnList;
			Settings.pNetworkConnectionsColumnList	= &NetworkConnectionsColumnList;
			Settings.pPrintersColumnList			= &PrintersColumnList;
			Settings.pRealFolderColumnList			= &RealFolderColumnList;
			Settings.pRecycleBinColumnList			= &RecycleBinColumnList;

			/* High-level settings. */
			SetDefaultTabSettings(&TabInfo);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("Locked"),(LPDWORD)&TabInfo.bLocked);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("AddressLocked"),(LPDWORD)&TabInfo.bAddressLocked);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("UseCustomName"),(LPDWORD)&TabInfo.bUseCustomName);
			NRegistrySettings::ReadStringFromRegistry(hTabKey,_T("CustomName"),TabInfo.szName,SIZEOF_ARRAY(TabInfo.szName));

			hr = CreateNewTab(pidlDirectory,&Settings,&TabInfo,TRUE,NULL);

			if(hr == S_OK)
				nTabsCreated++;

			CoTaskMemFree((LPVOID)pidlDirectory);
			RegCloseKey(hTabKey);

			i++;

			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
				_T("%d"),i);

			ReturnValue = RegOpenKeyEx(hKey,szItemKey,0,KEY_READ,&hTabKey);
		}

		RegCloseKey(hKey);
	}

	return nTabsCreated;
}

void Explorerplusplus::SaveColumnWidthsToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::list<Column_t> *pColumns)
{
	typedef struct
	{
		unsigned int id;
		int iWidth;
	} ColumnWidth_t;

	std::list<Column_t>::iterator	itr;
	ColumnWidth_t				*pColumnList = NULL;
	int							iColumn = 0;

	pColumnList = (ColumnWidth_t *)malloc(pColumns->size() * sizeof(ColumnWidth_t));

	for(itr = pColumns->begin();itr != pColumns->end();itr++)
	{
		pColumnList[iColumn].id			= itr->id;
		pColumnList[iColumn].iWidth		= itr->iWidth;

		iColumn++;
	}

	RegSetValueEx(hColumnsKey,szKeyName,0,REG_BINARY,
		(LPBYTE)pColumnList,(DWORD)(pColumns->size() * sizeof(ColumnWidth_t)));

	free(pColumnList);
}

void Explorerplusplus::LoadColumnWidthsFromRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::list<Column_t> *pColumns)
{
	typedef struct
	{
		unsigned int id;
		int iWidth;
	} ColumnWidth_t;

	ColumnWidth_t	ColumnList[64];
	Column_t		Column;
	DWORD			dwSize;
	DWORD			dwType;
	LONG			ret;
	unsigned int	i = 0;

	dwType = REG_BINARY;
	dwSize = sizeof(ColumnList);

	ret = RegQueryValueEx(hColumnsKey,szKeyName,0,&dwType,(LPBYTE)ColumnList,
		&dwSize);

	if(ret == ERROR_SUCCESS)
	{
		for(i = 0;i < dwSize / sizeof(ColumnWidth_t);i++)
		{
			Column.id = ColumnList[i].id;
			Column.iWidth = ColumnList[i].iWidth;

			pColumns->push_back(Column);
		}
	}
}

void Explorerplusplus::SaveColumnToRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::list<Column_t> *pColumns)
{
	std::list<Column_t>::iterator	itr;
	ColumnOld_t					*pColumnList = NULL;
	int							iColumn = 0;

	pColumnList = (ColumnOld_t *)malloc(pColumns->size() * sizeof(ColumnOld_t));

	for(itr = pColumns->begin();itr != pColumns->end();itr++)
	{
		pColumnList[iColumn].id			= itr->id;
		pColumnList[iColumn].bChecked	= itr->bChecked;

		iColumn++;
	}

	RegSetValueEx(hColumnsKey,szKeyName,0,REG_BINARY,
		(LPBYTE)pColumnList,(DWORD)(pColumns->size() * sizeof(ColumnOld_t)));

	free(pColumnList);
}

void Explorerplusplus::LoadColumnFromRegistry(HKEY hColumnsKey, const TCHAR *szKeyName, std::list<Column_t> *pColumns)
{
	ColumnOld_t		ColumnList[64];
	Column_t		Column;
	DWORD			dwSize;
	DWORD			dwType;
	unsigned int	i = 0;

	dwType = REG_BINARY;
	dwSize = sizeof(ColumnList);

	RegQueryValueEx(hColumnsKey,szKeyName,0,&dwType,(LPBYTE)ColumnList,
		&dwSize);

	for(i = 0;i < dwSize / sizeof(ColumnOld_t);i++)
	{
		Column.id = ColumnList[i].id;
		Column.bChecked = ColumnList[i].bChecked;
		Column.iWidth = DEFAULT_COLUMN_WIDTH;

		pColumns->push_back(Column);
	}
}

void Explorerplusplus::SaveDefaultColumnsToRegistry(void)
{
	HKEY			hColumnsKey;
	DWORD			Disposition;
	LONG			ReturnValue;

	/* Open/Create the main key that is used to store data. */
	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_COLUMNS_KEY,0,NULL,
	REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hColumnsKey,&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		SaveColumnToRegistry(hColumnsKey,_T("ControlPanelColumns"),&m_ControlPanelColumnList);
		SaveColumnWidthsToRegistry(hColumnsKey,_T("ControlPanelColumnWidths"),&m_ControlPanelColumnList);

		SaveColumnToRegistry(hColumnsKey,_T("MyComputerColumns"),&m_MyComputerColumnList);
		SaveColumnWidthsToRegistry(hColumnsKey,_T("MyComputerColumnWidths"),&m_MyComputerColumnList);

		SaveColumnToRegistry(hColumnsKey,_T("RealFolderColumns"),&m_RealFolderColumnList);
		SaveColumnWidthsToRegistry(hColumnsKey,_T("RealFolderColumnWidths"),&m_RealFolderColumnList);

		SaveColumnToRegistry(hColumnsKey,_T("RecycleBinColumns"),&m_RecycleBinColumnList);
		SaveColumnWidthsToRegistry(hColumnsKey,_T("RecycleBinColumnWidths"),&m_RecycleBinColumnList);

		SaveColumnToRegistry(hColumnsKey,_T("PrinterColumns"),&m_PrintersColumnList);
		SaveColumnWidthsToRegistry(hColumnsKey,_T("PrinterColumnWidths"),&m_PrintersColumnList);

		SaveColumnToRegistry(hColumnsKey,_T("NetworkColumns"),&m_NetworkConnectionsColumnList);
		SaveColumnWidthsToRegistry(hColumnsKey,_T("NetworkColumnWidths"),&m_NetworkConnectionsColumnList);

		SaveColumnToRegistry(hColumnsKey,_T("NetworkPlacesColumns"),&m_MyNetworkPlacesColumnList);
		SaveColumnWidthsToRegistry(hColumnsKey,_T("NetworkPlacesColumnWidths"),&m_MyNetworkPlacesColumnList);

		RegCloseKey(hColumnsKey);
	}
}

void Explorerplusplus::LoadDefaultColumnsFromRegistry(void)
{
	HKEY	hColumnsKey;
	LONG	res;

	/* Open/Create the main key that is used to store data. */
	res = RegOpenKeyEx(HKEY_CURRENT_USER,REG_COLUMNS_KEY,0,KEY_READ,&hColumnsKey);

	if(res == ERROR_SUCCESS)
	{
		m_ControlPanelColumnList.clear();
		m_MyComputerColumnList.clear();
		m_RealFolderColumnList.clear();
		m_RecycleBinColumnList.clear();
		m_PrintersColumnList.clear();
		m_NetworkConnectionsColumnList.clear();
		m_MyNetworkPlacesColumnList.clear();

		LoadColumnFromRegistry(hColumnsKey,_T("ControlPanelColumns"),&m_ControlPanelColumnList);
		LoadColumnFromRegistry(hColumnsKey,_T("MyComputerColumns"),&m_MyComputerColumnList);
		LoadColumnFromRegistry(hColumnsKey,_T("RealFolderColumns"),&m_RealFolderColumnList);
		LoadColumnFromRegistry(hColumnsKey,_T("RecycleBinColumns"),&m_RecycleBinColumnList);
		LoadColumnFromRegistry(hColumnsKey,_T("PrinterColumns"),&m_PrintersColumnList);
		LoadColumnFromRegistry(hColumnsKey,_T("NetworkColumns"),&m_NetworkConnectionsColumnList);
		LoadColumnFromRegistry(hColumnsKey,_T("NetworkPlacesColumns"),&m_MyNetworkPlacesColumnList);

		std::list<Column_t>	RealFolderColumnListTemp;
		std::list<Column_t>	MyComputerColumnListTemp;
		std::list<Column_t>	ControlPanelColumnListTemp;
		std::list<Column_t>	RecycleBinColumnListTemp;
		std::list<Column_t>	PrintersColumnListTemp;
		std::list<Column_t>	NetworkConnectionsColumnListTemp;
		std::list<Column_t>	MyNetworkPlacesColumnListTemp;

		LoadColumnWidthsFromRegistry(hColumnsKey,_T("ControlPanelColumnWidths"),&ControlPanelColumnListTemp);
		LoadColumnWidthsFromRegistry(hColumnsKey,_T("MyComputerColumnWidths"),&MyComputerColumnListTemp);
		LoadColumnWidthsFromRegistry(hColumnsKey,_T("RealFolderColumnWidths"),&RealFolderColumnListTemp);
		LoadColumnWidthsFromRegistry(hColumnsKey,_T("RecycleBinColumnWidths"),&RecycleBinColumnListTemp);
		LoadColumnWidthsFromRegistry(hColumnsKey,_T("PrinterColumnWidths"),&PrintersColumnListTemp);
		LoadColumnWidthsFromRegistry(hColumnsKey,_T("NetworkColumnWidths"),&NetworkConnectionsColumnListTemp);
		LoadColumnWidthsFromRegistry(hColumnsKey,_T("NetworkPlacesColumnWidths"),&MyNetworkPlacesColumnListTemp);

		UpdateColumnWidths(&m_ControlPanelColumnList,&ControlPanelColumnListTemp);
		UpdateColumnWidths(&m_MyComputerColumnList,&MyComputerColumnListTemp);
		UpdateColumnWidths(&m_RealFolderColumnList,&RealFolderColumnListTemp);
		UpdateColumnWidths(&m_RecycleBinColumnList,&RecycleBinColumnListTemp);
		UpdateColumnWidths(&m_PrintersColumnList,&PrintersColumnListTemp);
		UpdateColumnWidths(&m_NetworkConnectionsColumnList,&NetworkConnectionsColumnListTemp);
		UpdateColumnWidths(&m_MyNetworkPlacesColumnList,&MyNetworkPlacesColumnListTemp);

		ValidateColumns();

		RegCloseKey(hColumnsKey);
	}

	return;
}

void Explorerplusplus::SaveToolbarInformationToRegistry(void)
{
	HKEY	hKey;
	HKEY	hToolbarKey;
	REBARBANDINFO rbi;
	TCHAR	szItemKey[128];
	DWORD	Disposition;
	LONG	ReturnValue;
	int		nBands = 0;
	int		i = 0;

	/* First, delete any current rebar key. */
	SHDeleteKey(HKEY_CURRENT_USER,REG_TOOLBARS_KEY);

	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_TOOLBARS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		nBands = (int)SendMessage(m_hMainRebar,RB_GETBANDCOUNT,0,0);

		/* Use RBBIM_ID to map between windows and bands. */
		for(i = 0;i < nBands;i++)
		{
			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
				_T("%d"),i);

			ReturnValue = RegCreateKeyEx(hKey,szItemKey,0,NULL,
				REG_OPTION_NON_VOLATILE,KEY_WRITE,
				NULL,&hToolbarKey,&Disposition);

			if(ReturnValue == ERROR_SUCCESS)
			{
				rbi.cbSize = sizeof(rbi);
				rbi.fMask = RBBIM_ID|RBBIM_CHILD|RBBIM_SIZE|RBBIM_STYLE;
				SendMessage(m_hMainRebar,RB_GETBANDINFO,i,(LPARAM)&rbi);

				NRegistrySettings::SaveDwordToRegistry(hToolbarKey,_T("id"),rbi.wID);
				NRegistrySettings::SaveDwordToRegistry(hToolbarKey,_T("Style"),rbi.fStyle);
				NRegistrySettings::SaveDwordToRegistry(hToolbarKey,_T("Length"),rbi.cx);

				RegCloseKey(hToolbarKey);
			}
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadToolbarInformationFromRegistry(void)
{
	HKEY				hKey;
	HKEY				hToolbarKey;
	TCHAR				szItemKey[128];
	LONG				ReturnValue;
	int					i = 0;

	ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_TOOLBARS_KEY,0,KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
			_T("%d"),i);

		ReturnValue = RegOpenKeyEx(hKey,szItemKey,0,KEY_READ,&hToolbarKey);

		while(ReturnValue == ERROR_SUCCESS)
		{
			BOOL bUseChevron = FALSE;

			if(m_ToolbarInformation[i].fStyle & RBBS_USECHEVRON)
				bUseChevron = TRUE;

			NRegistrySettings::ReadDwordFromRegistry(hToolbarKey,_T("id"),
				(LPDWORD)&m_ToolbarInformation[i].wID);
			NRegistrySettings::ReadDwordFromRegistry(hToolbarKey,_T("Style"),
				(LPDWORD)&m_ToolbarInformation[i].fStyle);
			NRegistrySettings::ReadDwordFromRegistry(hToolbarKey,_T("Length"),
				(LPDWORD)&m_ToolbarInformation[i].cx);

			if(bUseChevron)
				m_ToolbarInformation[i].fStyle |= RBBS_USECHEVRON;

			RegCloseKey(hToolbarKey);

			i++;

			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
				_T("%d"),i);

			ReturnValue = RegOpenKeyEx(hKey,szItemKey,0,KEY_READ,&hToolbarKey);
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadApplicationToolbarFromRegistry()
{
	HKEY hKey;
	LONG ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_APPLICATIONS_KEY,0,KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		CApplicationToolbarPersistentSettings::GetInstance().LoadRegistrySettings(hKey);

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::SaveApplicationToolbarToRegistry()
{
	SHDeleteKey(HKEY_CURRENT_USER,REG_APPLICATIONS_KEY);

	HKEY hKey;
	LONG ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_APPLICATIONS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,NULL);

	if(ReturnValue == ERROR_SUCCESS)
	{
		CApplicationToolbarPersistentSettings::GetInstance().SaveRegistrySettings(hKey);

		RegCloseKey(hKey);
	}
}

Explorerplusplus::CLoadSaveRegistry::CLoadSaveRegistry(Explorerplusplus *pContainer) :
m_pContainer(pContainer)
{
	 
}

Explorerplusplus::CLoadSaveRegistry::~CLoadSaveRegistry()
{

}

void Explorerplusplus::CLoadSaveRegistry::LoadGenericSettings()
{
	m_pContainer->LoadSettings();
}

void Explorerplusplus::CLoadSaveRegistry::LoadBookmarks()
{
	m_pContainer->LoadBookmarksFromRegistry();
}

int Explorerplusplus::CLoadSaveRegistry::LoadPreviousTabs()
{
	return m_pContainer->LoadTabSettingsFromRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::LoadDefaultColumns()
{
	m_pContainer->LoadDefaultColumnsFromRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::LoadApplicationToolbar()
{
	m_pContainer->LoadApplicationToolbarFromRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::LoadToolbarInformation()
{
	m_pContainer->LoadToolbarInformationFromRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::LoadColorRules()
{
	NColorRuleHelper::LoadColorRulesFromRegistry(m_pContainer->m_ColorRules);
}

void Explorerplusplus::CLoadSaveRegistry::LoadDialogStates()
{
	m_pContainer->LoadDialogStatesFromRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveGenericSettings()
{
	m_pContainer->SaveSettings();
}

void Explorerplusplus::CLoadSaveRegistry::SaveBookmarks()
{
	m_pContainer->SaveBookmarksToRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveTabs()
{
	m_pContainer->SaveTabSettingsToRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveDefaultColumns()
{
	m_pContainer->SaveDefaultColumnsToRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveApplicationToolbar()
{
	m_pContainer->SaveApplicationToolbarToRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveToolbarInformation()
{
	m_pContainer->SaveToolbarInformationToRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveColorRules()
{
	NColorRuleHelper::SaveColorRulesToRegistry(m_pContainer->m_ColorRules);
}

void Explorerplusplus::CLoadSaveRegistry::SaveDialogStates()
{
	m_pContainer->SaveDialogStatesToRegistry();
}