// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "ApplicationToolbar.h"
#include "ColorRuleHelper.h"
#include "Config.h"
#include "DefaultColumns.h"
#include "Explorer++_internal.h"
#include "MainToolbar.h"
#include "../DisplayWindow/DisplayWindow.h"
#include "../Helper/Bookmark.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/Macros.h"
#include <boost/range/adaptor/map.hpp>

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
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowExtensions"), m_config->globalFolderSettings.showExtensions);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowStatusBar"), m_config->showStatusBar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowFolders"), m_config->showFolders);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowAddressBar"),m_config->showAddressBar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowToolbar"),m_config->showMainToolbar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowBookmarksToolbar"),m_config->showBookmarksToolbar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowDrivesToolbar"),m_config->showDrivesToolbar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowApplicationToolbar"),m_config->showApplicationToolbar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowFullTitlePath"),m_config->showFullTitlePath.get());
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("AlwaysOpenNewTab"),m_config->alwaysOpenNewTab);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("TreeViewWidth"), m_config->treeViewWidth);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowFriendlyDates"), m_config->globalFolderSettings.showFriendlyDates);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowDisplayWindow"),m_config->showDisplayWindow);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowFolderSizes"),m_config->globalFolderSettings.showFolderSizes);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("DisableFolderSizesNetworkRemovable"),m_config->globalFolderSettings.disableFolderSizesNetworkRemovable);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("StartupMode"), m_config->startupMode);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("NextToCurrent"),m_config->openNewTabNextToCurrent);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ConfirmCloseTabs"), m_config->confirmCloseTabs);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowInfoTips"), m_config->showInfoTips);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("InfoTipType"), m_config->infoTipType);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("TreeViewDelayEnabled"),m_config->treeViewDelayEnabled);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("LockToolbars"), m_config->lockToolbars);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ExtendTabControl"),m_config->extendTabControl);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("UseFullRowSelect"),m_config->useFullRowSelect);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowFilePreviews"),m_config->showFilePreviews);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ReplaceExplorerMode"), m_config->replaceExplorerMode);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowUserNameTitleBar"),m_config->showUserNameInTitleBar.get());
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("AllowMultipleInstances"),m_config->allowMultipleInstances);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("OneClickActivate"),m_config->globalFolderSettings.oneClickActivate);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("OneClickActivateHoverTime"),m_config->globalFolderSettings.oneClickActivateHoverTime);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ForceSameTabWidth"),m_config->forceSameTabWidth);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("DoubleClickTabClose"),m_config->doubleClickTabClose);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("HandleZipFiles"),m_config->handleZipFiles);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("InsertSorted"),m_config->insertSorted);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowPrivilegeLevelInTitleBar"),m_config->showPrivilegeLevelInTitleBar.get());
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("AlwaysShowTabBar"),m_config->alwaysShowTabBar);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("CheckBoxSelection"),m_config->checkBoxSelection);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ForceSize"),m_config->globalFolderSettings.forceSize);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("SizeDisplayFormat"),m_config->globalFolderSettings.sizeDisplayFormat);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("CloseMainWindowOnTabClose"),m_config->closeMainWindowOnTabClose);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowTabBarAtBottom"), m_config->showTabBarAtBottom);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("OverwriteExistingFilesConfirmation"),m_config->overwriteExistingFilesConfirmation);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("LargeToolbarIcons"),m_config->useLargeToolbarIcons);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("PlayNavigationSound"),m_config->playNavigationSound);

		NRegistrySettings::SaveStringToRegistry(hSettingsKey,_T("NewTabDirectory"), m_config->defaultTabDirectory.c_str());

		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("Language"),m_Language);

		/* Global settings. */
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowHiddenGlobal"), m_config->defaultFolderSettings.showHidden);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ViewModeGlobal"), m_config->defaultFolderSettings.viewMode);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowGridlinesGlobal"), m_config->globalFolderSettings.showGridlines);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowInGroupsGlobal"), m_config->defaultFolderSettings.showInGroups);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("AutoArrangeGlobal"), m_config->defaultFolderSettings.autoArrange);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("SortAscendingGlobal"), m_config->defaultFolderSettings.sortAscending);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("HideSystemFilesGlobal"),m_config->globalFolderSettings.hideSystemFiles);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("HideLinkExtensionGlobal"), m_config->globalFolderSettings.hideLinkExtension);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("ShowTaskbarThumbnails"), m_config->showTaskbarThumbnails);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("SynchronizeTreeview"), m_config->synchronizeTreeview);
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("TVAutoExpandSelected"), m_config->treeViewAutoExpandSelected);

		/* Display window settings. */
		NRegistrySettings::SaveDwordToRegistry(hSettingsKey,_T("DisplayWindowHeight"), m_config->displayWindowHeight);

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

		/* TODO: This should
		be done within the
		main toolbar class. */
		tbSave.hkr			= HKEY_CURRENT_USER;
		tbSave.pszSubKey	= NExplorerplusplus::REG_SETTINGS_KEY;
		tbSave.pszValueName	= _T("ToolbarState");

		SendMessage(m_mainToolbar->GetHWND(),TB_SAVERESTORE,TRUE,(LPARAM)&tbSave);

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
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowExtensions"),(LPDWORD)&m_config->globalFolderSettings.showExtensions);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowStatusBar"),(LPDWORD)&m_config->showStatusBar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowFolders"),(LPDWORD)&m_config->showFolders);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowAddressBar"),(LPDWORD)&m_config->showAddressBar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowToolbar"),(LPDWORD)&m_config->showMainToolbar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowBookmarksToolbar"),(LPDWORD)&m_config->showBookmarksToolbar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowDrivesToolbar"),(LPDWORD)&m_config->showDrivesToolbar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowApplicationToolbar"),(LPDWORD)&m_config->showApplicationToolbar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("AlwaysOpenNewTab"),(LPDWORD)&m_config->alwaysOpenNewTab);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("TreeViewWidth"),(LPDWORD)&m_config->treeViewWidth);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowFriendlyDates"),(LPDWORD)&m_config->globalFolderSettings.showFriendlyDates);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowDisplayWindow"),(LPDWORD)&m_config->showDisplayWindow);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowFolderSizes"),(LPDWORD)&m_config->globalFolderSettings.showFolderSizes);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("DisableFolderSizesNetworkRemovable"),(LPDWORD)&m_config->globalFolderSettings.disableFolderSizesNetworkRemovable);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("StartupMode"),(LPDWORD)&m_config->startupMode);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("NextToCurrent"),(LPDWORD)&m_config->openNewTabNextToCurrent);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ConfirmCloseTabs"),(LPDWORD)&m_config->confirmCloseTabs);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowInfoTips"),(LPDWORD)&m_config->showInfoTips);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("InfoTipType"),(LPDWORD)&m_config->infoTipType);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("TreeViewDelayEnabled"),(LPDWORD)&m_config->treeViewDelayEnabled);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("LockToolbars"),(LPDWORD)&m_config->lockToolbars);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ExtendTabControl"),(LPDWORD)&m_config->extendTabControl);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("UseFullRowSelect"),(LPDWORD)&m_config->useFullRowSelect);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowFilePreviews"),(LPDWORD)&m_config->showFilePreviews);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ReplaceExplorerMode"),(LPDWORD)&m_config->replaceExplorerMode);

		DWORD numericValue;
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey, _T("ShowFullTitlePath"), &numericValue);
		m_config->showFullTitlePath.set(numericValue);

		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey, _T("ShowUserNameTitleBar"), &numericValue);
		m_config->showUserNameInTitleBar.set(numericValue);

		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey, _T("ShowPrivilegeLevelInTitleBar"), &numericValue);
		m_config->showPrivilegeLevelInTitleBar.set(numericValue);

		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("AllowMultipleInstances"),(LPDWORD)&m_config->allowMultipleInstances);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("OneClickActivate"),(LPDWORD)&m_config->globalFolderSettings.oneClickActivate);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("OneClickActivateHoverTime"),(LPDWORD)&m_config->globalFolderSettings.oneClickActivateHoverTime);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ForceSameTabWidth"),(LPDWORD)&m_config->forceSameTabWidth);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("DoubleClickTabClose"),(LPDWORD)&m_config->doubleClickTabClose);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("HandleZipFiles"),(LPDWORD)&m_config->handleZipFiles);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("InsertSorted"),(LPDWORD)&m_config->insertSorted);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("AlwaysShowTabBar"),(LPDWORD)&m_config->alwaysShowTabBar);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("CheckBoxSelection"),(LPDWORD)&m_config->checkBoxSelection);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ForceSize"),(LPDWORD)&m_config->globalFolderSettings.forceSize);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("SizeDisplayFormat"),(LPDWORD)&m_config->globalFolderSettings.sizeDisplayFormat);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("CloseMainWindowOnTabClose"),(LPDWORD)&m_config->closeMainWindowOnTabClose);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowTabBarAtBottom"),(LPDWORD)&m_config->showTabBarAtBottom);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowTaskbarThumbnails"),(LPDWORD)&m_config->showTaskbarThumbnails);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("SynchronizeTreeview"),(LPDWORD)&m_config->synchronizeTreeview);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("TVAutoExpandSelected"),(LPDWORD)&m_config->treeViewAutoExpandSelected);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("OverwriteExistingFilesConfirmation"),(LPDWORD)&m_config->overwriteExistingFilesConfirmation);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("LargeToolbarIcons"),(LPDWORD)&m_config->useLargeToolbarIcons);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("PlayNavigationSound"),(LPDWORD)&m_config->playNavigationSound);

		TCHAR value[MAX_PATH];
		NRegistrySettings::ReadStringFromRegistry(hSettingsKey,_T("NewTabDirectory"),value,SIZEOF_ARRAY(value));
		m_config->defaultTabDirectory = value;

		lStatus = NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("Language"),(LPDWORD)&m_Language);

		if(lStatus == ERROR_SUCCESS)
			m_bLanguageLoaded = TRUE;

		/* Global settings. */
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowHiddenGlobal"),(LPDWORD)&m_config->defaultFolderSettings.showHidden);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ViewModeGlobal"),(LPDWORD)&m_config->defaultFolderSettings.viewMode);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowGridlinesGlobal"),(LPDWORD)&m_config->globalFolderSettings.showGridlines);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("ShowInGroupsGlobal"),(LPDWORD)&m_config->defaultFolderSettings.showInGroups);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("AutoArrangeGlobal"),(LPDWORD)&m_config->defaultFolderSettings.autoArrange);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("SortAscendingGlobal"),(LPDWORD)&m_config->defaultFolderSettings.sortAscending);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("HideSystemFilesGlobal"),(LPDWORD)&m_config->globalFolderSettings.hideSystemFiles);
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("HideLinkExtensionGlobal"),(LPDWORD)&m_config->globalFolderSettings.hideLinkExtension);

		/* Display window settings. */
		NRegistrySettings::ReadDwordFromRegistry(hSettingsKey,_T("DisplayWindowHeight"),(LPDWORD)&m_config->displayWindowHeight);

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
	TCHAR	szItemKey[128];
	LPITEMIDLIST	pidlDirectory = NULL;
	UINT	ViewMode;
	UINT	SortMode;
	DWORD	Disposition;
	LONG	ReturnValue;

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
		int tabNum = 0;

		for (auto &tab : GetAllTabs() | boost::adaptors::map_values)
		{
			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
				_T("%d"),tabNum);

			ReturnValue = RegCreateKeyEx(hKey,szItemKey,0,NULL,
				REG_OPTION_NON_VOLATILE,KEY_WRITE,
				NULL,&hTabKey,&Disposition);

			if(ReturnValue == ERROR_SUCCESS)
			{
				pidlDirectory = tab.GetShellBrowser()->QueryCurrentDirectoryIdl();
				RegSetValueEx(hTabKey,_T("Directory"),0,REG_BINARY,
					(LPBYTE)pidlDirectory,ILGetSize(pidlDirectory));
				CoTaskMemFree((LPVOID)pidlDirectory);

				ViewMode = tab.GetShellBrowser()->GetViewMode();

				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ViewMode"),ViewMode);

				SortMode = tab.GetShellBrowser()->GetSortMode();
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("SortMode"),SortMode);

				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("SortAscending"), tab.GetShellBrowser()->GetSortAscending());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ShowInGroups"), tab.GetShellBrowser()->GetShowInGroups());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ApplyFilter"), tab.GetShellBrowser()->GetFilterStatus());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("FilterCaseSensitive"), tab.GetShellBrowser()->GetFilterCaseSensitive());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("ShowHidden"), tab.GetShellBrowser()->GetShowHidden());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("AutoArrange"), tab.GetShellBrowser()->GetAutoArrange());

				TCHAR szFilter[512];

				tab.GetShellBrowser()->GetFilter(szFilter,SIZEOF_ARRAY(szFilter));
				NRegistrySettings::SaveStringToRegistry(hTabKey,_T("Filter"),szFilter);

				/* Now save the tabs columns. */
				ReturnValue = RegCreateKeyEx(hTabKey,_T("Columns"),
					0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hColumnsKey,
					&Disposition);

				if(ReturnValue == ERROR_SUCCESS)
				{
					ColumnExport_t cie;

					tab.GetShellBrowser()->ExportAllColumns(&cie);

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
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("Locked"),tab.GetLocked());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("AddressLocked"),tab.GetAddressLocked());
				NRegistrySettings::SaveDwordToRegistry(hTabKey,_T("UseCustomName"),tab.GetUseCustomName());

				if(tab.GetUseCustomName())
					NRegistrySettings::SaveStringToRegistry(hTabKey,_T("CustomName"),tab.GetName().c_str());
				else
					NRegistrySettings::SaveStringToRegistry(hTabKey,_T("CustomName"),EMPTY_STRING);

				RegCloseKey(hTabKey);
			}

			tabNum++;
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

			FolderSettings folderSettings;

			DWORD value;
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("ViewMode"),&value);
			folderSettings.viewMode = ViewMode::_from_integral(value);

			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("SortMode"),&value);
			folderSettings.sortMode = SortMode::_from_integral(value);

			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("SortAscending"),(LPDWORD)&folderSettings.sortAscending);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("ShowInGroups"),(LPDWORD)&folderSettings.showInGroups);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("ApplyFilter"),(LPDWORD)&folderSettings.applyFilter);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("FilterCaseSensitive"),(LPDWORD)&folderSettings.filterCaseSensitive);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("ShowHidden"),(LPDWORD)&folderSettings.showHidden);
			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("AutoArrange"),(LPDWORD)&folderSettings.autoArrange);

			TCHAR filter[512];
			NRegistrySettings::ReadStringFromRegistry(hTabKey,_T("Filter"),filter,SIZEOF_ARRAY(filter));
			folderSettings.filter = filter;

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

			InitialColumns initialColumns;

			initialColumns.pControlPanelColumnList			= &ControlPanelColumnList;
			initialColumns.pMyComputerColumnList			= &MyComputerColumnList;
			initialColumns.pMyNetworkPlacesColumnList		= &MyNetworkPlacesColumnList;
			initialColumns.pNetworkConnectionsColumnList	= &NetworkConnectionsColumnList;
			initialColumns.pPrintersColumnList				= &PrintersColumnList;
			initialColumns.pRealFolderColumnList			= &RealFolderColumnList;
			initialColumns.pRecycleBinColumnList			= &RecycleBinColumnList;

			TabSettings tabSettings;

			tabSettings.selected = true;

			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("Locked"),&value);
			tabSettings.locked = value;

			NRegistrySettings::ReadDwordFromRegistry(hTabKey,_T("AddressLocked"),&value);
			tabSettings.addressLocked = value;

			TCHAR customName[64];
			NRegistrySettings::ReadStringFromRegistry(hTabKey,_T("CustomName"),customName,SIZEOF_ARRAY(customName));
			tabSettings.name = customName;

			hr = CreateNewTab(pidlDirectory, tabSettings, &folderSettings, &initialColumns);

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