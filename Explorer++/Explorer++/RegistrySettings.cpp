/******************************************************************
 *
 * Project: Explorer++
 * File: Settings.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Saves and loads all main program settings.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "SearchDialog.h"
#include "WildcardSelectDialog.h"
#include "SetFileAttributesDialog.h"
#include "RenameTabDialog.h"
#include "MassRenameDialog.h"
#include "FilterDialog.h"
#include "ColorRuleDialog.h"
#include "CustomizeColorsDialog.h"
#include "SplitFileDialog.h"
#include "DestroyFilesDialog.h"
#include "MergeFilesDialog.h"
#include "SelectColumnsDialog.h"
#include "SetDefaultColumnsDialog.h"
#include "../Helper/RegistrySettings.h"


#define REG_BOOKMARKS_KEY			_T("Software\\Explorer++\\Bookmarks")
#define REG_TABS_KEY				_T("Software\\Explorer++\\Tabs")
#define REG_TOOLBARS_KEY			_T("Software\\Explorer++\\Toolbars")
#define REG_COLUMNS_KEY				_T("Software\\Explorer++\\DefaultColumns")
#define REG_APPLICATIONS_KEY		_T("Software\\Explorer++\\ApplicationToolbar")
#define REG_DIALOGS_KEY				_T("Software\\Explorer++\\Dialogs")
#define REG_COLORS_KEY				_T("Software\\Explorer++\\ColorRules")

TCHAR pszDirectoriesKey[]	= _T("Directories");
TCHAR pszCustomDirsKey[]	= _T("Custom Directories");

extern DWORD g_Language;

BOOL LoadWindowPosition(WINDOWPLACEMENT *pwndpl)
{
	HKEY hSettingsKey;
	BOOL bRes = FALSE;

	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,REG_SETTINGS_KEY,0,
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
	}

	return bRes;
}

BOOL LoadAllowMultipleInstancesFromRegistry(void)
{
	BOOL bAllowMultipleInstances = TRUE;

	HKEY hSettingsKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,REG_SETTINGS_KEY,0,KEY_READ,&hSettingsKey);

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
	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_SETTINGS_KEY,0,NULL,
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
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("HideRecycleBinGlobal"),m_bHideRecycleBinGlobal);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("HideSysVolInfoGlobal"),m_bHideSysVolInfoGlobal);

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
		tbSave.pszSubKey	= REG_SETTINGS_KEY;
		tbSave.pszValueName	= _T("ToolbarState");

		SendMessage(m_hMainToolbar,TB_SAVERESTORE,TRUE,(LPARAM)&tbSave);

		RegCloseKey(hSettingsKey);
	}

	return ReturnValue;
}

LONG Explorerplusplus::LoadSettings(LPCTSTR KeyPath)
{
	HKEY			hSettingsKey;
	LONG			ReturnValue;
	LONG			CentreColorStatus = TRUE;
	LONG			SurroundColorStatus = TRUE;
	LONG			TextColorStatus = TRUE;
	LONG			FontStatus = TRUE;
	LONG			lStatus;

	/* Open/Create the main key that is used to store data. */
	ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_SETTINGS_KEY,0,KEY_READ,&hSettingsKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		/* User setiings. */
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
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("HideRecycleBinGlobal"),(LPDWORD)&m_bHideRecycleBinGlobal);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("HideSysVolInfoGlobal"),(LPDWORD)&m_bHideSysVolInfoGlobal);

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
	HKEY			hBookmarksKey;
	Bookmark_t		RootBookmark;
	Bookmark_t		FirstChild;
	DWORD			Disposition;
	HRESULT			hr;
	LONG			ReturnValue;

	/* First, delete the 'Bookmarks' key, along
	with all of its subkeys. */
	SHDeleteKey(HKEY_CURRENT_USER,REG_BOOKMARKS_KEY);

	/* Simply open the 'main' registry key, as the 'bookmarks' sub-key
	will be created by the root bookmark (called 'Bookmarks'). */
	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_BOOKMARKS_KEY,
	0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hBookmarksKey,
	&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		m_Bookmark.GetRoot(&RootBookmark);

		hr = m_Bookmark.GetChild(&RootBookmark,&FirstChild);

		if(SUCCEEDED(hr))
		{
			SaveBookmarksToRegistryInternal(hBookmarksKey,&FirstChild,0);
		}

		RegCloseKey(hBookmarksKey);
	}
}

void Explorerplusplus::SaveBookmarksToRegistryInternal(HKEY hKey,
Bookmark_t *pBookmark,int count)
{
	HKEY		hKeyChild;
	Bookmark_t	FirstChild;
	Bookmark_t	SiblingBookmark;
	TCHAR		szKeyName[32];
	DWORD		Disposition;
	HRESULT		hr;
	LONG		ReturnValue;

	_itow_s(count,szKeyName,SIZEOF_ARRAY(szKeyName),10);
	ReturnValue = RegCreateKeyEx(hKey,szKeyName,0,NULL,
		REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKeyChild,
		&Disposition);

	NRegistrySettings::SaveStringToRegistry(hKeyChild,_T("Name"),pBookmark->szItemName);
	NRegistrySettings::SaveStringToRegistry(hKeyChild,_T("Description"),pBookmark->szItemDescription);
	NRegistrySettings::SaveDwordToRegistry(hKeyChild,_T("Type"),pBookmark->Type);
	NRegistrySettings::SaveDwordToRegistry(hKeyChild,_T("ShowOnBookmarksToolbar"),pBookmark->bShowOnToolbar);

	count++;

	if(pBookmark->Type == BOOKMARK_TYPE_BOOKMARK)
	{
		NRegistrySettings::SaveStringToRegistry(hKeyChild,_T("Location"),pBookmark->szLocation);
	}
	
	if(pBookmark->Type == BOOKMARK_TYPE_FOLDER)
	{
		hr = m_Bookmark.GetChild(pBookmark,&FirstChild);

		if(SUCCEEDED(hr))
		{
			SaveBookmarksToRegistryInternal(hKeyChild,&FirstChild,0);
		}
	}

	RegCloseKey(hKeyChild);

	hr = m_Bookmark.GetNextBookmarkSibling(pBookmark,&SiblingBookmark);

	if(SUCCEEDED(hr))
	{
		SaveBookmarksToRegistryInternal(hKey,&SiblingBookmark,count);
	}

	return;
}

void Explorerplusplus::LoadBookmarksFromRegistry(void)
{
	HKEY		hBookmarksKey;
	Bookmark_t	RootBookmark;
	LONG		ReturnValue;

	ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_BOOKMARKS_KEY,
		0,KEY_READ,&hBookmarksKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		m_Bookmark.GetRoot(&RootBookmark);

		LoadBookmarksFromRegistryInternal(hBookmarksKey,RootBookmark.pHandle);

		RegCloseKey(hBookmarksKey);
	}
}

void Explorerplusplus::LoadBookmarksFromRegistryInternal(HKEY hBookmarks,void *ParentFolder)
{
	HKEY	hKeyChild;
	Bookmark_t	NewBookmark;
	TCHAR	szKeyName[256];
	DWORD	dwKeyLength;
	LONG	lTypeStatus;
	LONG	lNameStatus;
	LONG	lDescriptionStatus;
	LONG	lToolbarStatus;
	LONG	lLocationStatus;
	DWORD	dwIndex = 0;

	dwKeyLength = SIZEOF_ARRAY(szKeyName);

	/* Enumerate each of the subkeys. */
	while(RegEnumKeyEx(hBookmarks,dwIndex++,szKeyName,&dwKeyLength,NULL,NULL,NULL,NULL) == ERROR_SUCCESS)
	{
		/* Open the subkey. First, attempt to load
		the type. If there is no type specifier, ignore
		the key. If any other values are missing, also
		ignore the key. */
		RegOpenKeyEx(hBookmarks,szKeyName,0,KEY_READ,&hKeyChild);

		lTypeStatus = NRegistrySettings::ReadDwordFromRegistry(hKeyChild,_T("Type"),
			(LPDWORD)&NewBookmark.Type);
		lNameStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild,_T("Name"),
			NewBookmark.szItemName,SIZEOF_ARRAY(NewBookmark.szItemName));
		lToolbarStatus = NRegistrySettings::ReadDwordFromRegistry(hKeyChild,_T("ShowOnBookmarksToolbar"),
			(LPDWORD)&NewBookmark.bShowOnToolbar);
		lDescriptionStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild,_T("Description"),
			NewBookmark.szItemDescription,SIZEOF_ARRAY(NewBookmark.szItemDescription));

		if(lTypeStatus == ERROR_SUCCESS && lNameStatus == ERROR_SUCCESS &&
			lToolbarStatus == ERROR_SUCCESS)
		{
			if(NewBookmark.Type == BOOKMARK_TYPE_FOLDER)
			{
				/* Create the bookmark folder. */
				m_Bookmark.CreateNewBookmark(ParentFolder,&NewBookmark);

				LoadBookmarksFromRegistryInternal(hKeyChild,NewBookmark.pHandle);
			}
			else
			{
				lLocationStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild,_T("Location"),
					NewBookmark.szLocation,SIZEOF_ARRAY(NewBookmark.szLocation));

				m_Bookmark.CreateNewBookmark(ParentFolder,&NewBookmark);
			}
		}

		RegCloseKey(hKeyChild);

		dwKeyLength = SIZEOF_ARRAY(szKeyName);
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

				m_pFolderView[(int)tcItem.lParam]->GetCurrentViewMode(&ViewMode);

				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ViewMode"),ViewMode);

				m_pFolderView[(int)tcItem.lParam]->GetSortMode(&SortMode);
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("SortMode"),SortMode);

				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("SortAscending"),m_pFolderView[(int)tcItem.lParam]->IsSortAscending());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ShowInGroups"),m_pFolderView[(int)tcItem.lParam]->IsGroupViewEnabled());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ApplyFilter"),m_pShellBrowser[(int)tcItem.lParam]->GetFilterStatus());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("FilterCaseSensitive"),m_pShellBrowser[(int)tcItem.lParam]->GetFilterCaseSensitive());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ShowHidden"),m_pShellBrowser[(int)tcItem.lParam]->QueryShowHidden());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("AutoArrange"),m_pFolderView[(int)tcItem.lParam]->GetAutoArrange());
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
				}

				/* High-level settings. */
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("Locked"),m_TabInfo[(int)tcItem.lParam].bLocked);
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("AddressLocked"),m_TabInfo[(int)tcItem.lParam].bAddressLocked);
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("UseCustomName"),m_TabInfo[(int)tcItem.lParam].bUseCustomName);

				if(m_TabInfo[(int)tcItem.lParam].bUseCustomName)
					NRegistrySettings::SaveStringToRegistry(hTabKey,_T("CustomName"),m_TabInfo[(int)tcItem.lParam].szName);
				else
					NRegistrySettings::SaveStringToRegistry(hTabKey,_T("CustomName"),EMPTY_STRING);

				RegCloseKey(hTabKey);
			}
		}

		RegCloseKey(hKey);
	}
}

void UpdateColumnWidths(list<Column_t> *pColumnList,list<Column_t> *pColumnWidthList)
{
	list<Column_t>::iterator itr1;
	list<Column_t>::iterator itr2;

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

			list<Column_t>	RealFolderColumnList;
			list<Column_t>	MyComputerColumnList;
			list<Column_t>	ControlPanelColumnList;
			list<Column_t>	RecycleBinColumnList;
			list<Column_t>	PrintersColumnList;
			list<Column_t>	NetworkConnectionsColumnList;
			list<Column_t>	MyNetworkPlacesColumnList;

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

				list<Column_t>	RealFolderColumnListTemp;
				list<Column_t>	MyComputerColumnListTemp;
				list<Column_t>	ControlPanelColumnListTemp;
				list<Column_t>	RecycleBinColumnListTemp;
				list<Column_t>	PrintersColumnListTemp;
				list<Column_t>	NetworkConnectionsColumnListTemp;
				list<Column_t>	MyNetworkPlacesColumnListTemp;

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

void Explorerplusplus::SaveColumnWidthsToRegistry(HKEY hColumnsKey,TCHAR *szKeyName,list<Column_t> *pColumns)
{
	typedef struct
	{
		unsigned int id;
		int iWidth;
	} ColumnWidth_t;

	list<Column_t>::iterator	itr;
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

void Explorerplusplus::LoadColumnWidthsFromRegistry(HKEY hColumnsKey,TCHAR *szKeyName,list<Column_t> *pColumns)
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

void Explorerplusplus::SaveColumnToRegistry(HKEY hColumnsKey,TCHAR *szKeyName,list<Column_t> *pColumns)
{
	list<Column_t>::iterator	itr;
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

void Explorerplusplus::LoadColumnFromRegistry(HKEY hColumnsKey,TCHAR *szKeyName,list<Column_t> *pColumns)
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

		list<Column_t>	RealFolderColumnListTemp;
		list<Column_t>	MyComputerColumnListTemp;
		list<Column_t>	ControlPanelColumnListTemp;
		list<Column_t>	RecycleBinColumnListTemp;
		list<Column_t>	PrintersColumnListTemp;
		list<Column_t>	NetworkConnectionsColumnListTemp;
		list<Column_t>	MyNetworkPlacesColumnListTemp;

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

void Explorerplusplus::SaveApplicationToolbarToRegistry(void)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	/* First, delete the 'ApplicationToolbar' key, along
	with all of its subkeys. */
	SHDeleteKey(HKEY_CURRENT_USER,REG_APPLICATIONS_KEY);

	/* Open/Create the main key that is used to store data. */
	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_APPLICATIONS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_pAppButtons != NULL)
			SaveApplicationToolbarToRegistryInternal(hKey,m_pAppButtons,0);

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::SaveApplicationToolbarToRegistryInternal(HKEY hKey,
ApplicationButton_t	*pab,int count)
{
	HKEY				hKeyChild;
	TCHAR				szKeyName[32];
	DWORD				Disposition;
	LONG				ReturnValue;

	_itow_s(count,szKeyName,SIZEOF_ARRAY(szKeyName),10);
	ReturnValue = RegCreateKeyEx(hKey,szKeyName,0,NULL,
		REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKeyChild,
		&Disposition);

	NRegistrySettings::SaveStringToRegistry(hKeyChild,_T("Name"),pab->szName);
	NRegistrySettings::SaveStringToRegistry(hKeyChild,_T("Command"),pab->szCommand);
	NRegistrySettings::SaveDwordToRegistry(hKeyChild,_T("ShowNameOnToolbar"),pab->bShowNameOnToolbar);

	count++;

	RegCloseKey(hKeyChild);

	if(pab->pNext != NULL)
		SaveApplicationToolbarToRegistryInternal(hKey,pab->pNext,count);
}

void Explorerplusplus::LoadApplicationToolbarFromRegistry(void)
{
	HKEY		hKey;
	LONG		ReturnValue;

	ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_APPLICATIONS_KEY,
		0,KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		LoadApplicationToolbarFromRegistryInternal(hKey);

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadApplicationToolbarFromRegistryInternal(HKEY hKey)
{
	HKEY	hKeyChild;
	TCHAR	szItemKey[256];
	TCHAR	szName[512];
	TCHAR	szCommand[512];
	LONG	lNameStatus;
	LONG	lCommandStatus;
	LONG	ReturnValue;
	BOOL	bShowNameOnToolbar = TRUE;
	int		i = 0;

	StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
		_T("%d"),i);

	ReturnValue = RegOpenKeyEx(hKey,szItemKey,0,KEY_READ,&hKeyChild);

	while(ReturnValue == ERROR_SUCCESS)
	{
		lNameStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild,_T("Name"),
			szName,SIZEOF_ARRAY(szName));
		lCommandStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild,_T("Command"),
			szCommand,SIZEOF_ARRAY(szCommand));
		NRegistrySettings::ReadDwordFromRegistry(hKeyChild,_T("ShowNameOnToolbar"),
			(LPDWORD)&bShowNameOnToolbar);

		if(lNameStatus == ERROR_SUCCESS && lCommandStatus == ERROR_SUCCESS)
		{
			/* Create the application button. */
			ApplicationToolbarAddItem(szName,szCommand,bShowNameOnToolbar);
		}

		RegCloseKey(hKeyChild);

		i++;

		StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
			_T("%d"),i);

		ReturnValue = RegOpenKeyEx(hKey,szItemKey,0,KEY_READ,&hKeyChild);
	}
}

void Explorerplusplus::SaveColorRulesToRegistry(void)
{
	/* First, delete the 'ColorRules' key, along
	with all of its subkeys. */
	SHDeleteKey(HKEY_CURRENT_USER,REG_COLORS_KEY);

	HKEY hKey;
	DWORD Disposition;

	LONG lRes = RegCreateKeyEx(HKEY_CURRENT_USER,REG_COLORS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	int iCount = 0;

	if(lRes == ERROR_SUCCESS)
	{
		for each(auto ColorRule in m_ColorRuleList)
		{
			SaveColorRulesToRegistryInternal(hKey,&ColorRule,iCount);
			iCount++;
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::SaveColorRulesToRegistryInternal(HKEY hKey,
	ColorRule_t *pColorRule,int iCount)
{
	HKEY hKeyChild;
	TCHAR szKeyName[32];
	DWORD Disposition;

	_itow_s(iCount,szKeyName,SIZEOF_ARRAY(szKeyName),10);
	RegCreateKeyEx(hKey,szKeyName,0,NULL,
		REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKeyChild,
		&Disposition);

	NRegistrySettings::SaveStringToRegistry(hKeyChild,_T("Description"),pColorRule->strDescription.c_str());
	NRegistrySettings::SaveStringToRegistry(hKeyChild,_T("FilenamePattern"),pColorRule->strFilterPattern.c_str());
	NRegistrySettings::SaveDwordToRegistry(hKeyChild,_T("Attributes"),pColorRule->dwFilterAttributes);
	RegSetValueEx(hKeyChild,_T("Color"),0,REG_BINARY,reinterpret_cast<LPBYTE>(&pColorRule->rgbColour),
		sizeof(pColorRule->rgbColour));

	RegCloseKey(hKeyChild);
}

void Explorerplusplus::LoadColorRulesFromRegistry(void)
{
	HKEY hKey;

	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,REG_COLORS_KEY,
		0,KEY_READ,&hKey);

	if(lRes == ERROR_SUCCESS)
	{
		m_ColorRuleList.clear();
		LoadColorRulesFromRegistryInternal(hKey);
		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadColorRulesFromRegistryInternal(HKEY hKey)
{
	TCHAR szKeyName[256];
	DWORD dwIndex = 0;
	DWORD dwKeyLength = SIZEOF_ARRAY(szKeyName);

	while(RegEnumKeyEx(hKey,dwIndex++,szKeyName,&dwKeyLength,NULL,
		NULL,NULL,NULL) == ERROR_SUCCESS)
	{
		ColorRule_t ColorRule;
		HKEY hKeyChild;

		/* Open the subkey. First, attempt to load
		the type. If there is no type specifier, ignore
		the key. If any other values are missing, also
		ignore the key. */
		RegOpenKeyEx(hKey,szKeyName,0,KEY_READ,&hKeyChild);

		LONG lDescriptionStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild,_T("Description"),
			ColorRule.strDescription);
		LONG lFilenamePatternStatus = NRegistrySettings::ReadStringFromRegistry(hKeyChild,_T("FilenamePattern"),
			ColorRule.strFilterPattern);
		NRegistrySettings::ReadDwordFromRegistry(hKeyChild,_T("Attributes"),
			&ColorRule.dwFilterAttributes);

		DWORD dwType = REG_BINARY;
		DWORD dwSize = sizeof(ColorRule.rgbColour);
		RegQueryValueEx(hKeyChild,_T("Color"),0,&dwType,
			reinterpret_cast<LPBYTE>(&ColorRule.rgbColour),&dwSize);

		if(lDescriptionStatus == ERROR_SUCCESS && lFilenamePatternStatus == ERROR_SUCCESS)
		{
			m_ColorRuleList.push_back(ColorRule);
		}

		RegCloseKey(hKeyChild);

		dwKeyLength = SIZEOF_ARRAY(szKeyName);
	}
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

void Explorerplusplus::SaveStateToRegistry(void)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_DIALOGS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		SaveAddBookmarkStateToRegistry(hKey);
		SaveDisplayColorsStateToRegistry(hKey);
		SaveOrganizeBookmarksStateToRegistry(hKey);

		CSearchDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CWildcardSelectDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CSetFileAttributesDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CRenameTabDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CMassRenameDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CFilterDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CColorRuleDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CCustomizeColorsDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CSplitFileDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CDestroyFilesDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CMergeFilesDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CSelectColumnsDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);
		CSetDefaultColumnsDialogPersistentSettings::GetInstance().SaveRegistrySettings(hKey);

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::SaveAddBookmarkStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_ADDBOOKMARK_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bAddBookmarkDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptAddBookmark,
				sizeof(m_ptAddBookmark));
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::SaveDisplayColorsStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_DISPLAYCOLORS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bDisplayColorsDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptDisplayColors,
				sizeof(m_ptDisplayColors));
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::SaveOrganizeBookmarksStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_ORGANIZEBOOKMARKS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bOrganizeBookmarksDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptOrganizeBookmarks,
				sizeof(m_ptOrganizeBookmarks));
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadStateFromRegistry(void)
{
	HKEY				hKey;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_DIALOGS_KEY,0,KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		LoadAddBookmarkStateFromRegistry(hKey);
		LoadDisplayColorsStateFromRegistry(hKey);
		LoadOrganizeBookmarksStateFromRegistry(hKey);

		CSearchDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CWildcardSelectDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CSetFileAttributesDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CRenameTabDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CMassRenameDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CFilterDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CColorRuleDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CCustomizeColorsDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CSplitFileDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CDestroyFilesDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CMergeFilesDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CSelectColumnsDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);
		CSetDefaultColumnsDialogPersistentSettings::GetInstance().LoadRegistrySettings(hKey);

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadAddBookmarkStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_ADDBOOKMARK_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptAddBookmark,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bAddBookmarkDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadDisplayColorsStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_DISPLAYCOLORS_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptDisplayColors,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bDisplayColorsDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::LoadOrganizeBookmarksStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_ORGANIZEBOOKMARKS_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptOrganizeBookmarks,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bOrganizeBookmarksDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void Explorerplusplus::CLoadSaveRegistry::LoadGenericSettings(void)
{
	m_pContainer->LoadSettings(REG_MAIN_KEY);
}

void Explorerplusplus::CLoadSaveRegistry::LoadBookmarks(void)
{
	m_pContainer->LoadBookmarksFromRegistry();
}

int Explorerplusplus::CLoadSaveRegistry::LoadPreviousTabs(void)
{
	return m_pContainer->LoadTabSettingsFromRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::LoadDefaultColumns(void)
{
	m_pContainer->LoadDefaultColumnsFromRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::LoadApplicationToolbar(void)
{
	m_pContainer->LoadApplicationToolbarFromRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::LoadToolbarInformation(void)
{
	m_pContainer->LoadToolbarInformationFromRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::LoadColorRules(void)
{
	m_pContainer->LoadColorRulesFromRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::LoadState(void)
{
	m_pContainer->LoadStateFromRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveGenericSettings(void)
{
	m_pContainer->SaveSettings();
}

void Explorerplusplus::CLoadSaveRegistry::SaveBookmarks(void)
{
	m_pContainer->SaveBookmarksToRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveTabs(void)
{
	m_pContainer->SaveTabSettingsToRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveDefaultColumns(void)
{
	m_pContainer->SaveDefaultColumnsToRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveApplicationToolbar(void)
{
	m_pContainer->SaveApplicationToolbarToRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveToolbarInformation(void)
{
	m_pContainer->SaveToolbarInformationToRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveColorRules(void)
{
	m_pContainer->SaveColorRulesToRegistry();
}

void Explorerplusplus::CLoadSaveRegistry::SaveState(void)
{
	m_pContainer->SaveStateToRegistry();
}