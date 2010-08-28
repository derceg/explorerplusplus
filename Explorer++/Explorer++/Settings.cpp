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
#include "../Helper/Registry.h"

#define SMALL_SIZE		5

TCHAR pszDirectoriesKey[]	= _T("Directories");
TCHAR pszCustomDirsKey[]	= _T("Custom Directories");

extern DWORD g_Language;

LONG CContainer::SaveSettings(void)
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
		/* User settings. */
		SaveDwordToRegistry(hSettingsKey,_T("LastSelectedTab"),m_iLastSelectedTab);
		SaveDwordToRegistry(hSettingsKey,_T("ShowExtensions"),m_bShowExtensionsGlobal);
		SaveDwordToRegistry(hSettingsKey,_T("ShowStatusBar"),m_bShowStatusBar);
		SaveDwordToRegistry(hSettingsKey,_T("ShowFolders"),m_bShowFolders);
		SaveDwordToRegistry(hSettingsKey,_T("ShowAddressBar"),m_bShowAddressBar);
		SaveDwordToRegistry(hSettingsKey,_T("ShowToolbar"),m_bShowMainToolbar);
		SaveDwordToRegistry(hSettingsKey,_T("ShowBookmarksToolbar"),m_bShowBookmarksToolbar);
		SaveDwordToRegistry(hSettingsKey,_T("ShowDrivesToolbar"),m_bShowDrivesToolbar);
		SaveDwordToRegistry(hSettingsKey,_T("ShowApplicationToolbar"),m_bShowApplicationToolbar);
		SaveDwordToRegistry(hSettingsKey,_T("ShowFullTitlePath"),m_bShowFullTitlePath);
		SaveDwordToRegistry(hSettingsKey,_T("AlwaysOpenNewTab"),m_bAlwaysOpenNewTab);
		SaveDwordToRegistry(hSettingsKey,_T("TreeViewWidth"),m_TreeViewWidth);
		SaveDwordToRegistry(hSettingsKey,_T("ShowFriendlyDates"),m_bShowFriendlyDatesGlobal);
		SaveDwordToRegistry(hSettingsKey,_T("ShowDisplayWindow"),m_bShowDisplayWindow);
		SaveDwordToRegistry(hSettingsKey,_T("ShowFolderSizes"),m_bShowFolderSizes);
		SaveDwordToRegistry(hSettingsKey,_T("DisableFolderSizesNetworkRemovable"),m_bDisableFolderSizesNetworkRemovable);
		SaveDwordToRegistry(hSettingsKey,_T("StartupMode"),m_StartupMode);
		SaveDwordToRegistry(hSettingsKey,_T("NextToCurrent"),m_bOpenNewTabNextToCurrent);
		SaveDwordToRegistry(hSettingsKey,_T("ConfirmCloseTabs"),m_bConfirmCloseTabs);
		SaveDwordToRegistry(hSettingsKey,_T("ShowInfoTips"),m_bShowInfoTips);
		SaveDwordToRegistry(hSettingsKey,_T("InfoTipType"),m_InfoTipType);
		SaveDwordToRegistry(hSettingsKey,_T("TreeViewDelayEnabled"),m_bTreeViewDelayEnabled);
		SaveDwordToRegistry(hSettingsKey,_T("LockToolbars"),m_bLockToolbars);
		SaveDwordToRegistry(hSettingsKey,_T("ExtendTabControl"),m_bExtendTabControl);
		SaveDwordToRegistry(hSettingsKey,_T("UseFullRowSelect"),m_bUseFullRowSelect);
		SaveDwordToRegistry(hSettingsKey,_T("ShowFilePreviews"),m_bShowFilePreviews);
		SaveDwordToRegistry(hSettingsKey,_T("ReplaceExplorerMode"),m_ReplaceExplorerMode);
		SaveDwordToRegistry(hSettingsKey,_T("ShowUserNameTitleBar"),m_bShowUserNameInTitleBar);
		SaveDwordToRegistry(hSettingsKey,_T("AllowMultipleInstances"),m_bAllowMultipleInstances);
		SaveDwordToRegistry(hSettingsKey,_T("OneClickActivate"),m_bOneClickActivate);
		SaveDwordToRegistry(hSettingsKey,_T("ForceSameTabWidth"),m_bForceSameTabWidth);
		SaveDwordToRegistry(hSettingsKey,_T("DoubleClickTabClose"),m_bDoubleClickTabClose);
		SaveDwordToRegistry(hSettingsKey,_T("HandleZipFiles"),m_bHandleZipFiles);
		SaveDwordToRegistry(hSettingsKey,_T("InsertSorted"),m_bInsertSorted);
		SaveDwordToRegistry(hSettingsKey,_T("ShowPrivilegeLevelInTitleBar"),m_bShowPrivilegeLevelInTitleBar);
		SaveDwordToRegistry(hSettingsKey,_T("AlwaysShowTabBar"),m_bAlwaysShowTabBar);
		SaveDwordToRegistry(hSettingsKey,_T("CheckBoxSelection"),m_bCheckBoxSelection);
		SaveDwordToRegistry(hSettingsKey,_T("ForceSize"),m_bForceSize);
		SaveDwordToRegistry(hSettingsKey,_T("SizeDisplayFormat"),m_SizeDisplayFormat);
		SaveDwordToRegistry(hSettingsKey,_T("CloseMainWindowOnTabClose"),m_bCloseMainWindowOnTabClose);
		SaveDwordToRegistry(hSettingsKey,_T("ShowTabBarAtBottom"),m_bShowTabBarAtBottom);
		SaveDwordToRegistry(hSettingsKey,_T("OverwriteExistingFilesConfirmation"),m_bOverwriteExistingFilesConfirmation);
		SaveDwordToRegistry(hSettingsKey,_T("LargeToolbarIcons"),m_bLargeToolbarIcons);

		SaveStringToRegistry(hSettingsKey,_T("NewTabDirectory"),m_DefaultTabDirectory);

		SaveDwordToRegistry(hSettingsKey,_T("Language"),m_Language);

		/* Global settings. */
		SaveDwordToRegistry(hSettingsKey,_T("ShowHiddenGlobal"),m_bShowHiddenGlobal);
		SaveDwordToRegistry(hSettingsKey,_T("ViewModeGlobal"),m_ViewModeGlobal);
		SaveDwordToRegistry(hSettingsKey,_T("ShowGridlinesGlobal"),m_bShowGridlinesGlobal);
		SaveDwordToRegistry(hSettingsKey,_T("ShowInGroupsGlobal"),m_bShowInGroupsGlobal);
		SaveDwordToRegistry(hSettingsKey,_T("AutoArrangeGlobal"),m_bAutoArrangeGlobal);
		SaveDwordToRegistry(hSettingsKey,_T("SortAscendingGlobal"),m_bSortAscendingGlobal);
		SaveDwordToRegistry(hSettingsKey,_T("HideSystemFilesGlobal"),m_bHideSystemFilesGlobal);
		SaveDwordToRegistry(hSettingsKey,_T("HideLinkExtensionGlobal"),m_bHideLinkExtensionGlobal);
		SaveDwordToRegistry(hSettingsKey,_T("ShowTaskbarThumbnails"),m_bShowTaskbarThumbnails);
		SaveDwordToRegistry(hSettingsKey,_T("SynchronizeTreeview"),m_bSynchronizeTreeview);
		SaveDwordToRegistry(hSettingsKey,_T("TVAutoExpandSelected"),m_bTVAutoExpandSelected);

		/* Display window settings. */
		SaveDwordToRegistry(hSettingsKey,_T("DisplayWindowHeight"),m_DisplayWindowHeight);

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

LONG CContainer::LoadSettings(LPCTSTR KeyPath)
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
		ReadDwordFromRegistry(hSettingsKey,_T("LastSelectedTab"),(LPDWORD)&m_iLastSelectedTab);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowExtensions"),(LPDWORD)&m_bShowExtensionsGlobal);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowStatusBar"),(LPDWORD)&m_bShowStatusBar);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowFolders"),(LPDWORD)&m_bShowFolders);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowAddressBar"),(LPDWORD)&m_bShowAddressBar);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowToolbar"),(LPDWORD)&m_bShowMainToolbar);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowBookmarksToolbar"),(LPDWORD)&m_bShowBookmarksToolbar);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowDrivesToolbar"),(LPDWORD)&m_bShowDrivesToolbar);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowApplicationToolbar"),(LPDWORD)&m_bShowApplicationToolbar);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowFullTitlePath"),(LPDWORD)&m_bShowFullTitlePath);
		ReadDwordFromRegistry(hSettingsKey,_T("AlwaysOpenNewTab"),(LPDWORD)&m_bAlwaysOpenNewTab);
		ReadDwordFromRegistry(hSettingsKey,_T("TreeViewWidth"),(LPDWORD)&m_TreeViewWidth);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowFriendlyDates"),(LPDWORD)&m_bShowFriendlyDatesGlobal);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowDisplayWindow"),(LPDWORD)&m_bShowDisplayWindow);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowFolderSizes"),(LPDWORD)&m_bShowFolderSizes);
		ReadDwordFromRegistry(hSettingsKey,_T("DisableFolderSizesNetworkRemovable"),(LPDWORD)&m_bDisableFolderSizesNetworkRemovable);
		ReadDwordFromRegistry(hSettingsKey,_T("StartupMode"),(LPDWORD)&m_StartupMode);
		ReadDwordFromRegistry(hSettingsKey,_T("NextToCurrent"),(LPDWORD)&m_bOpenNewTabNextToCurrent);
		ReadDwordFromRegistry(hSettingsKey,_T("ConfirmCloseTabs"),(LPDWORD)&m_bConfirmCloseTabs);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowInfoTips"),(LPDWORD)&m_bShowInfoTips);
		ReadDwordFromRegistry(hSettingsKey,_T("InfoTipType"),(LPDWORD)&m_InfoTipType);
		ReadDwordFromRegistry(hSettingsKey,_T("TreeViewDelayEnabled"),(LPDWORD)&m_bTreeViewDelayEnabled);
		ReadDwordFromRegistry(hSettingsKey,_T("LockToolbars"),(LPDWORD)&m_bLockToolbars);
		ReadDwordFromRegistry(hSettingsKey,_T("ExtendTabControl"),(LPDWORD)&m_bExtendTabControl);
		ReadDwordFromRegistry(hSettingsKey,_T("UseFullRowSelect"),(LPDWORD)&m_bUseFullRowSelect);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowFilePreviews"),(LPDWORD)&m_bShowFilePreviews);
		ReadDwordFromRegistry(hSettingsKey,_T("ReplaceExplorerMode"),(LPDWORD)&m_ReplaceExplorerMode);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowUserNameTitleBar"),(LPDWORD)&m_bShowUserNameInTitleBar);
		ReadDwordFromRegistry(hSettingsKey,_T("AllowMultipleInstances"),(LPDWORD)&m_bAllowMultipleInstances);
		ReadDwordFromRegistry(hSettingsKey,_T("OneClickActivate"),(LPDWORD)&m_bOneClickActivate);
		ReadDwordFromRegistry(hSettingsKey,_T("ForceSameTabWidth"),(LPDWORD)&m_bForceSameTabWidth);
		ReadDwordFromRegistry(hSettingsKey,_T("DoubleClickTabClose"),(LPDWORD)&m_bDoubleClickTabClose);
		ReadDwordFromRegistry(hSettingsKey,_T("HandleZipFiles"),(LPDWORD)&m_bHandleZipFiles);
		ReadDwordFromRegistry(hSettingsKey,_T("InsertSorted"),(LPDWORD)&m_bInsertSorted);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowPrivilegeLevelInTitleBar"),(LPDWORD)&m_bShowPrivilegeLevelInTitleBar);
		ReadDwordFromRegistry(hSettingsKey,_T("AlwaysShowTabBar"),(LPDWORD)&m_bAlwaysShowTabBar);
		ReadDwordFromRegistry(hSettingsKey,_T("CheckBoxSelection"),(LPDWORD)&m_bCheckBoxSelection);
		ReadDwordFromRegistry(hSettingsKey,_T("ForceSize"),(LPDWORD)&m_bForceSize);
		ReadDwordFromRegistry(hSettingsKey,_T("SizeDisplayFormat"),(LPDWORD)&m_SizeDisplayFormat);
		ReadDwordFromRegistry(hSettingsKey,_T("CloseMainWindowOnTabClose"),(LPDWORD)&m_bCloseMainWindowOnTabClose);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowTabBarAtBottom"),(LPDWORD)&m_bShowTabBarAtBottom);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowTaskbarThumbnails"),(LPDWORD)&m_bShowTaskbarThumbnails);
		ReadDwordFromRegistry(hSettingsKey,_T("SynchronizeTreeview"),(LPDWORD)&m_bSynchronizeTreeview);
		ReadDwordFromRegistry(hSettingsKey,_T("TVAutoExpandSelected"),(LPDWORD)&m_bTVAutoExpandSelected);
		ReadDwordFromRegistry(hSettingsKey,_T("OverwriteExistingFilesConfirmation"),(LPDWORD)&m_bOverwriteExistingFilesConfirmation);
		ReadDwordFromRegistry(hSettingsKey,_T("LargeToolbarIcons"),(LPDWORD)&m_bLargeToolbarIcons);

		ReadStringFromRegistry(hSettingsKey,_T("NewTabDirectory"),m_DefaultTabDirectory,SIZEOF_ARRAY(m_DefaultTabDirectory));

		lStatus = ReadDwordFromRegistry(hSettingsKey,_T("Language"),(LPDWORD)&m_Language);

		if(lStatus == ERROR_SUCCESS)
			m_bLanguageLoaded = TRUE;

		/* Global settings. */
		ReadDwordFromRegistry(hSettingsKey,_T("ShowHiddenGlobal"),(LPDWORD)&m_bShowHiddenGlobal);
		ReadDwordFromRegistry(hSettingsKey,_T("ViewModeGlobal"),(LPDWORD)&m_ViewModeGlobal);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowGridlinesGlobal"),(LPDWORD)&m_bShowGridlinesGlobal);
		ReadDwordFromRegistry(hSettingsKey,_T("ShowInGroupsGlobal"),(LPDWORD)&m_bShowInGroupsGlobal);
		ReadDwordFromRegistry(hSettingsKey,_T("AutoArrangeGlobal"),(LPDWORD)&m_bAutoArrangeGlobal);
		ReadDwordFromRegistry(hSettingsKey,_T("SortAscendingGlobal"),(LPDWORD)&m_bSortAscendingGlobal);
		ReadDwordFromRegistry(hSettingsKey,_T("HideSystemFilesGlobal"),(LPDWORD)&m_bHideSystemFilesGlobal);
		ReadDwordFromRegistry(hSettingsKey,_T("HideLinkExtensionGlobal"),(LPDWORD)&m_bHideLinkExtensionGlobal);

		/* Display window settings. */
		ReadDwordFromRegistry(hSettingsKey,_T("DisplayWindowHeight"),(LPDWORD)&m_DisplayWindowHeight);

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

LONG CContainer::SaveFilters(void)
{
	HKEY						hFiltersKey;
	list<Filter_t>::iterator	itr;
	TCHAR						szKeyName[SMALL_SIZE];
	DWORD						Disposition;
	LONG						ReturnValue;
	int							i = 0;

	/* First, delete the key (so that any previous values are
	erased). */
	SHDeleteKey(HKEY_CURRENT_USER,REG_FILTERS_KEY);

	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_FILTERS_KEY,
	0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,
	NULL,&hFiltersKey,&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		for(itr = m_FilterList.begin();itr != m_FilterList.end();itr++)
		{
			StringCchPrintf(szKeyName,SIZEOF_ARRAY(szKeyName),_T("%d"),i);

			ReturnValue = RegSetValueEx(hFiltersKey,szKeyName,0,REG_SZ,
				(LPBYTE)itr->pszFilterString,lstrlen(itr->pszFilterString) * sizeof(TCHAR));

			i++;
		}

		/* Close all the keys used... */
		RegCloseKey(hFiltersKey);
	}

	return ReturnValue;
}

LONG CContainer::LoadFilters(void)
{
	HKEY		hFiltersKey;
	Filter_t	Filter;
	TCHAR		szKeyName[SMALL_SIZE];
	LONG		ReturnValue;
	DWORD		Type;
	DWORD		cbData;
	int			i = 0;

	m_FilterList.clear();

	ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_FILTERS_KEY,0,KEY_READ,&hFiltersKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		StringCchPrintf(szKeyName,SIZEOF_ARRAY(szKeyName),_T("%d"),i);

		while(RegQueryValueEx(hFiltersKey,szKeyName,0,NULL,NULL,&cbData)
			== ERROR_SUCCESS)
		{
			Filter.pszFilterString = (TCHAR *)malloc(cbData + 1);

			RegQueryValueEx(hFiltersKey,szKeyName,0,&Type,(LPBYTE)Filter.pszFilterString,&cbData);

			Filter.pszFilterString[lstrlen(Filter.pszFilterString) + 1] = '\0';

			m_FilterList.push_back(Filter);

			//free(Filter.pszFilterString);

			i++;
			StringCchPrintf(szKeyName,SIZEOF_ARRAY(szKeyName),_T("%d"),i);
		}
	}

	/* Close all the keys used... */
	RegCloseKey(hFiltersKey);

	/* Return the number of directory paths that were successfully read. */
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

void CContainer::SaveBookmarksToRegistry(void)
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

void CContainer::SaveBookmarksToRegistryInternal(HKEY hKey,
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

	SaveStringToRegistry(hKeyChild,_T("Name"),pBookmark->szItemName);
	SaveStringToRegistry(hKeyChild,_T("Description"),pBookmark->szItemDescription);
	SaveDwordToRegistry(hKeyChild,_T("Type"),pBookmark->Type);
	SaveDwordToRegistry(hKeyChild,_T("ShowOnBookmarksToolbar"),pBookmark->bShowOnToolbar);

	count++;

	if(pBookmark->Type == BOOKMARK_TYPE_BOOKMARK)
	{
		SaveStringToRegistry(hKeyChild,_T("Location"),pBookmark->szLocation);
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

void CContainer::LoadBookmarksFromRegistry(void)
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

void CContainer::LoadBookmarksFromRegistryInternal(HKEY hBookmarks,void *ParentFolder)
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

		lTypeStatus = ReadDwordFromRegistry(hKeyChild,_T("Type"),
			(LPDWORD)&NewBookmark.Type);
		lNameStatus = ReadStringFromRegistry(hKeyChild,_T("Name"),
			NewBookmark.szItemName,SIZEOF_ARRAY(NewBookmark.szItemName));
		lToolbarStatus = ReadDwordFromRegistry(hKeyChild,_T("ShowOnBookmarksToolbar"),
			(LPDWORD)&NewBookmark.bShowOnToolbar);
		lDescriptionStatus = ReadStringFromRegistry(hKeyChild,_T("Description"),
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
				lLocationStatus = ReadStringFromRegistry(hKeyChild,_T("Location"),
					NewBookmark.szLocation,SIZEOF_ARRAY(NewBookmark.szLocation));

				m_Bookmark.CreateNewBookmark(ParentFolder,&NewBookmark);
			}
		}

		RegCloseKey(hKeyChild);

		dwKeyLength = SIZEOF_ARRAY(szKeyName);
	}
}

void CContainer::SaveTabSettingsToRegistry(void)
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

				SaveDwordToRegistry(hTabKey,_T("ViewMode"),ViewMode);

				m_pFolderView[(int)tcItem.lParam]->GetSortMode(&SortMode);
				SaveDwordToRegistry(hTabKey,_T("SortMode"),SortMode);

				SaveDwordToRegistry(hTabKey,_T("SortAscending"),m_pFolderView[(int)tcItem.lParam]->IsSortAscending());
				SaveDwordToRegistry(hTabKey,_T("ShowInGroups"),m_pFolderView[(int)tcItem.lParam]->IsGroupViewEnabled());
				SaveDwordToRegistry(hTabKey,_T("ApplyFilter"),m_pShellBrowser[(int)tcItem.lParam]->GetFilterStatus());
				SaveDwordToRegistry(hTabKey,_T("ShowHidden"),m_pShellBrowser[(int)tcItem.lParam]->QueryShowHidden());
				SaveDwordToRegistry(hTabKey,_T("AutoArrange"),m_pFolderView[(int)tcItem.lParam]->GetAutoArrange());
				SaveDwordToRegistry(hTabKey,_T("ShowGridlines"),m_pShellBrowser[(int)tcItem.lParam]->QueryGridlinesActive());

				TCHAR szFilter[512];

				m_pShellBrowser[(int)tcItem.lParam]->GetFilter(szFilter,SIZEOF_ARRAY(szFilter));
				SaveStringToRegistry(hTabKey,_T("Filter"),szFilter);

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
				SaveDwordToRegistry(hTabKey,_T("Locked"),m_TabInfo[(int)tcItem.lParam].bLocked);
				SaveDwordToRegistry(hTabKey,_T("AddressLocked"),m_TabInfo[(int)tcItem.lParam].bAddressLocked);
				SaveDwordToRegistry(hTabKey,_T("UseCustomName"),m_TabInfo[(int)tcItem.lParam].bUseCustomName);

				if(m_TabInfo[(int)tcItem.lParam].bUseCustomName)
					SaveStringToRegistry(hTabKey,_T("CustomName"),m_TabInfo[(int)tcItem.lParam].szName);
				else
					SaveStringToRegistry(hTabKey,_T("CustomName"),EMPTY_STRING);

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

int CContainer::LoadTabSettingsFromRegistry(void)
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

			ReadDwordFromRegistry(hTabKey,_T("ViewMode"),(LPDWORD)&Settings.ViewMode);

			ReadDwordFromRegistry(hTabKey,_T("SortMode"),(LPDWORD)&Settings.SortMode);
			ReadDwordFromRegistry(hTabKey,_T("SortAscending"),(LPDWORD)&Settings.bSortAscending);
			ReadDwordFromRegistry(hTabKey,_T("ShowInGroups"),(LPDWORD)&Settings.bShowInGroups);
			ReadDwordFromRegistry(hTabKey,_T("ApplyFilter"),(LPDWORD)&Settings.bApplyFilter);
			ReadDwordFromRegistry(hTabKey,_T("ShowHidden"),(LPDWORD)&Settings.bShowHidden);
			ReadDwordFromRegistry(hTabKey,_T("AutoArrange"),(LPDWORD)&Settings.bAutoArrange);
			ReadDwordFromRegistry(hTabKey,_T("ShowGridlines"),(LPDWORD)&Settings.bGridlinesActive);
			ReadStringFromRegistry(hTabKey,_T("Filter"),Settings.szFilter,SIZEOF_ARRAY(Settings.szFilter));

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
			ReadDwordFromRegistry(hTabKey,_T("Locked"),(LPDWORD)&TabInfo.bLocked);
			ReadDwordFromRegistry(hTabKey,_T("AddressLocked"),(LPDWORD)&TabInfo.bAddressLocked);
			ReadDwordFromRegistry(hTabKey,_T("UseCustomName"),(LPDWORD)&TabInfo.bUseCustomName);
			ReadStringFromRegistry(hTabKey,_T("CustomName"),TabInfo.szName,SIZEOF_ARRAY(TabInfo.szName));

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

void CContainer::SaveColumnWidthsToRegistry(HKEY hColumnsKey,TCHAR *szKeyName,list<Column_t> *pColumns)
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

void CContainer::LoadColumnWidthsFromRegistry(HKEY hColumnsKey,TCHAR *szKeyName,list<Column_t> *pColumns)
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

void CContainer::SaveColumnToRegistry(HKEY hColumnsKey,TCHAR *szKeyName,list<Column_t> *pColumns)
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

void CContainer::LoadColumnFromRegistry(HKEY hColumnsKey,TCHAR *szKeyName,list<Column_t> *pColumns)
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

void CContainer::SaveDefaultColumnsToRegistry(void)
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

void CContainer::LoadDefaultColumnsFromRegistry(void)
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

void CContainer::SaveApplicationToolbarToRegistry(void)
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

void CContainer::SaveApplicationToolbarToRegistryInternal(HKEY hKey,
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

	SaveStringToRegistry(hKeyChild,_T("Name"),pab->szName);
	SaveStringToRegistry(hKeyChild,_T("Command"),pab->szCommand);
	SaveDwordToRegistry(hKeyChild,_T("ShowNameOnToolbar"),pab->bShowNameOnToolbar);

	count++;

	RegCloseKey(hKeyChild);

	if(pab->pNext != NULL)
		SaveApplicationToolbarToRegistryInternal(hKey,pab->pNext,count);
}

void CContainer::LoadApplicationToolbarFromRegistry(void)
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

void CContainer::LoadApplicationToolbarFromRegistryInternal(HKEY hKey)
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
		lNameStatus = ReadStringFromRegistry(hKeyChild,_T("Name"),
			szName,SIZEOF_ARRAY(szName));
		lCommandStatus = ReadStringFromRegistry(hKeyChild,_T("Command"),
			szCommand,SIZEOF_ARRAY(szCommand));
		ReadDwordFromRegistry(hKeyChild,_T("ShowNameOnToolbar"),
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

void CContainer::SaveColorRulesToRegistry(void)
{
	HKEY	hKey;
	list<ListViewColouring_t>::iterator	itr;
	DWORD	Disposition;
	LONG	ReturnValue;
	int		iCount = 0;

	/* First, delete the 'ColorRules' key, along
	with all of its subkeys. */
	SHDeleteKey(HKEY_CURRENT_USER,REG_COLORS_KEY);

	/* Open/Create the main key that is used to store data. */
	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_COLORS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(!m_ColourFilter.empty())
		{
			for(itr = m_ColourFilter.begin();itr != m_ColourFilter.end();itr++)
			{
				SaveColorRulesToRegistryInternal(hKey,&(*itr),iCount);
				iCount++;
			}
		}

		RegCloseKey(hKey);
	}
}

void CContainer::SaveColorRulesToRegistryInternal(HKEY hKey,
ListViewColouring_t *plvc,int iCount)
{
	HKEY				hKeyChild;
	TCHAR				szKeyName[32];
	DWORD				Disposition;
	LONG				ReturnValue;

	_itow_s(iCount,szKeyName,SIZEOF_ARRAY(szKeyName),10);
	ReturnValue = RegCreateKeyEx(hKey,szKeyName,0,NULL,
		REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKeyChild,
		&Disposition);

	SaveStringToRegistry(hKeyChild,_T("Description"),plvc->szDescription);
	SaveStringToRegistry(hKeyChild,_T("FilenamePattern"),plvc->szFilterPattern);
	SaveDwordToRegistry(hKeyChild,_T("Attributes"),plvc->dwFilterAttributes);
	RegSetValueEx(hKeyChild,_T("Color"),0,REG_BINARY,
		(LPBYTE)&plvc->rgbColour,sizeof(plvc->rgbColour));

	RegCloseKey(hKeyChild);
}

void CContainer::LoadColorRulesFromRegistry(void)
{
	HKEY		hKey;
	LONG		ReturnValue;

	ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_COLORS_KEY,
		0,KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		m_ColourFilter.clear();

		LoadColorRulesFromRegistryInternal(hKey);

		RegCloseKey(hKey);
	}
}

void CContainer::LoadColorRulesFromRegistryInternal(HKEY hKey)
{
	HKEY	hKeyChild;
	TCHAR	szKeyName[256];
	ListViewColouring_t	lvc;
	DWORD	dwKeyLength;
	LONG	lDescriptionStatus;
	LONG	lFilenamePatternStatus;
	DWORD	dwIndex = 0;
	DWORD	dwType;
	DWORD	dwSize;

	dwKeyLength = SIZEOF_ARRAY(szKeyName);

	/* Enumerate each of the subkeys. */
	while(RegEnumKeyEx(hKey,dwIndex++,szKeyName,&dwKeyLength,NULL,
		NULL,NULL,NULL) == ERROR_SUCCESS)
	{
		/* Open the subkey. First, attempt to load
		the type. If there is no type specifier, ignore
		the key. If any other values are missing, also
		ignore the key. */
		RegOpenKeyEx(hKey,szKeyName,0,KEY_READ,&hKeyChild);

		lDescriptionStatus = ReadStringFromRegistry(hKeyChild,_T("Description"),
			lvc.szDescription,SIZEOF_ARRAY(lvc.szDescription));
		lFilenamePatternStatus = ReadStringFromRegistry(hKeyChild,_T("FilenamePattern"),
			lvc.szFilterPattern,SIZEOF_ARRAY(lvc.szFilterPattern));
		ReadDwordFromRegistry(hKeyChild,_T("Attributes"),
			(LPDWORD)&lvc.dwFilterAttributes);

		dwType = REG_BINARY;
		dwSize = sizeof(lvc.rgbColour);

		RegQueryValueEx(hKeyChild,_T("Color"),0,
			&dwType,(LPBYTE)&lvc.rgbColour,&dwSize);

		if(lDescriptionStatus == ERROR_SUCCESS && lFilenamePatternStatus == ERROR_SUCCESS)
		{
			/* Add the color rule to the global list. */
			m_ColourFilter.push_back(lvc);
		}

		RegCloseKey(hKeyChild);

		dwKeyLength = SIZEOF_ARRAY(szKeyName);
	}
}

void CContainer::SaveToolbarInformationToRegistry(void)
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

				SaveDwordToRegistry(hToolbarKey,_T("id"),rbi.wID);
				SaveDwordToRegistry(hToolbarKey,_T("Style"),rbi.fStyle);
				SaveDwordToRegistry(hToolbarKey,_T("Length"),rbi.cx);

				RegCloseKey(hToolbarKey);
			}
		}

		RegCloseKey(hKey);
	}
}

void CContainer::LoadToolbarInformationFromRegistry(void)
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

			ReadDwordFromRegistry(hToolbarKey,_T("id"),
				(LPDWORD)&m_ToolbarInformation[i].wID);
			ReadDwordFromRegistry(hToolbarKey,_T("Style"),
				(LPDWORD)&m_ToolbarInformation[i].fStyle);
			ReadDwordFromRegistry(hToolbarKey,_T("Length"),
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

void CContainer::SaveStateToRegistry(void)
{
	HKEY	hKey;
	list<WildcardSelectInfo_t>::iterator itr;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(HKEY_CURRENT_USER,REG_STATE_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		SaveAddBookmarkStateToRegistry(hKey);
		SaveColorRulesStateToRegistry(hKey);
		SaveCustomizeColorsStateToRegistry(hKey);
		SaveDestroyFilesStateToRegistry(hKey);
		SaveDisplayColorsStateToRegistry(hKey);
		SaveFilterStateToRegistry(hKey);
		SaveMassRenameStateToRegistry(hKey);
		SaveMergeFilesStateToRegistry(hKey);
		SaveOrganizeBookmarksStateToRegistry(hKey);
		SaveSearchStateToRegistry(hKey);
		SaveSelectColumnsStateToRegistry(hKey);
		SaveSelectDefaultColumnsStateToRegistry(hKey);
		SaveSetFileAttributesStateToRegistry(hKey);
		SaveSplitFileColumnsStateToRegistry(hKey);
		SaveWildcardStateToRegistry(hKey);

		RegCloseKey(hKey);
	}
}

void CContainer::SaveAddBookmarkStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	list<WildcardSelectInfo_t>::iterator itr;
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

void CContainer::SaveColorRulesStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_COLORRULES_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey,_T("CustomColors"),0,
			REG_BINARY,(LPBYTE)&m_ccCustomColors,
			sizeof(m_ccCustomColors));

		RegCloseKey(hKey);
	}
}

void CContainer::SaveCustomizeColorsStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	list<WildcardSelectInfo_t>::iterator itr;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_CUSTOMIZECOLORS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bCustomizeColorsDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptCustomizeColors,
				sizeof(m_ptCustomizeColors));

			RegSetValueEx(hKey,_T("InitialColor"),0,
				REG_BINARY,(LPBYTE)&m_crInitialColor,
				sizeof(m_crInitialColor));
		}

		RegCloseKey(hKey);
	}
}

void CContainer::SaveDestroyFilesStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_DESTROYFILES_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bDestroyFilesDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptDestroyFiles,
				sizeof(m_ptDestroyFiles));
		}

		RegCloseKey(hKey);
	}
}

void CContainer::SaveDisplayColorsStateToRegistry(HKEY hParentKey)
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

void CContainer::SaveFilterStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_FILTER_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bFilterDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptFilter,
				sizeof(m_ptFilter));
		}

		RegCloseKey(hKey);
	}
}

void CContainer::SaveMassRenameStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_MASSRENAME_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bMassRenameDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptMassRename,
				sizeof(m_ptMassRename));
		}

		RegCloseKey(hKey);
	}
}

void CContainer::SaveMergeFilesStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_MERGEFILES_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bMergeFilesDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptMergeFiles,
				sizeof(m_ptMergeFiles));
		}

		RegCloseKey(hKey);
	}
}

void CContainer::SaveOrganizeBookmarksStateToRegistry(HKEY hParentKey)
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

void CContainer::SaveSearchStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	list<SearchDirectoryInfo_t>::iterator itr;
	list<SearchPatternInfo_t>::iterator itr2;
	TCHAR	szItemKey[128];
	DWORD	Disposition;
	LONG	ReturnValue;
	int		i = 0;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_SEARCH_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bSearchDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptSearch,sizeof(m_ptSearch));

			SaveDwordToRegistry(hKey,_T("Width"),m_iSearchWidth);
			SaveDwordToRegistry(hKey,_T("Height"),m_iSearchHeight);

			SaveDwordToRegistry(hKey,_T("ColumnWidth1"),m_iColumnWidth1);
			SaveDwordToRegistry(hKey,_T("ColumnWidth2"),m_iColumnWidth2);

			for(itr = m_SearchDirectories.begin();itr != m_SearchDirectories.end();itr++)
			{
				StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),_T("Directory%d"),i++);

				SaveStringToRegistry(hKey,szItemKey,itr->szDirectory);
			}

			i = 0;

			for(itr2 = m_SearchPatterns.begin();itr2 != m_SearchPatterns.end();itr2++)
			{
				StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),_T("Pattern%d"),i++);

				SaveStringToRegistry(hKey,szItemKey,itr2->szPattern);
			}

			SaveStringToRegistry(hKey,_T("SearchDirectoryText"),m_SearchPatternText);

			SaveDwordToRegistry(hKey,_T("SearchSubFolders"),m_bSearchSubFolders);
		}

		RegCloseKey(hKey);
	}
}

void CContainer::SaveSelectColumnsStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_SELECTCOLUMNS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bSelectColumnsDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptSelectColumns,
				sizeof(m_ptSelectColumns));
		}

		RegCloseKey(hKey);
	}
}

void CContainer::SaveSelectDefaultColumnsStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_SELECTDEFAULTCOLUMNS_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bSetDefaultColumnsDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptSetDefaultColumns,
				sizeof(m_ptSetDefaultColumns));
		}

		RegCloseKey(hKey);
	}
}

void CContainer::SaveSetFileAttributesStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_SETFILEATTRIBUTES_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bSetFileAttributesDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptSetFileAttributes,
				sizeof(m_ptSetFileAttributes));
		}

		RegCloseKey(hKey);
	}
}

void CContainer::SaveSplitFileColumnsStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	DWORD	Disposition;
	LONG	ReturnValue;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_SPLITFILE_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bSplitFileDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptSplitFile,
				sizeof(m_ptSplitFile));
		}

		RegCloseKey(hKey);
	}
}

void CContainer::SaveWildcardStateToRegistry(HKEY hParentKey)
{
	HKEY	hKey;
	list<WildcardSelectInfo_t>::iterator itr;
	TCHAR	szItemKey[128];
	DWORD	Disposition;
	LONG	ReturnValue;
	int		i = 0;

	ReturnValue = RegCreateKeyEx(hParentKey,REG_WILDCARD_KEY,
		0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,
		&Disposition);

	if(ReturnValue == ERROR_SUCCESS)
	{
		if(m_bWildcardDlgStateSaved)
		{
			RegSetValueEx(hKey,_T("Position"),0,
				REG_BINARY,(LPBYTE)&m_ptWildcardSelect,sizeof(m_ptWildcardSelect));

			for(itr = m_wsiList.begin();itr != m_wsiList.end();itr++)
			{
				StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),_T("Pattern%d"),i++);

				SaveStringToRegistry(hKey,szItemKey,itr->szPattern);
			}

			SaveStringToRegistry(hKey,_T("CurrentText"),m_szwsiText);
		}

		RegCloseKey(hKey);
	}
}

void CContainer::LoadStateFromRegistry(void)
{
	HKEY				hKey;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(HKEY_CURRENT_USER,REG_STATE_KEY,0,KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		LoadAddBookmarkStateFromRegistry(hKey);
		LoadColorRulesStateFromRegistry(hKey);
		LoadCustomizeColorsStateFromRegistry(hKey);
		LoadDestroyFilesStateFromRegistry(hKey);
		LoadDisplayColorsStateFromRegistry(hKey);
		LoadFilterStateFromRegistry(hKey);
		LoadMassRenameStateFromRegistry(hKey);
		LoadMergeFilesStateFromRegistry(hKey);
		LoadOrganizeBookmarksStateFromRegistry(hKey);
		LoadSearchStateFromRegistry(hKey);
		LoadSelectColumnsStateFromRegistry(hKey);
		LoadSelectDefaultColumnsStateFromRegistry(hKey);
		LoadSetFileAttributesStateFromRegistry(hKey);
		LoadSplitFileStateFromRegistry(hKey);
		LoadWildcardStateFromRegistry(hKey);

		RegCloseKey(hKey);
	}
}

void CContainer::LoadAddBookmarkStateFromRegistry(HKEY hParentKey)
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

void CContainer::LoadColorRulesStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_COLORRULES_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(m_ccCustomColors);
		ReturnValue = RegQueryValueEx(hKey,_T("CustomColors"),
			NULL,NULL,(LPBYTE)&m_ccCustomColors,&dwSize);

		RegCloseKey(hKey);
	}
}

void CContainer::LoadCustomizeColorsStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_CUSTOMIZECOLORS_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptCustomizeColors,&dwSize);

		dwSize = sizeof(m_crInitialColor);
		RegQueryValueEx(hKey,_T("InitialColor"),
			NULL,NULL,(LPBYTE)&m_crInitialColor,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bCustomizeColorsDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void CContainer::LoadDestroyFilesStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_DESTROYFILES_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptDestroyFiles,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bDestroyFilesDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void CContainer::LoadDisplayColorsStateFromRegistry(HKEY hParentKey)
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

void CContainer::LoadFilterStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_FILTER_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptFilter,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bFilterDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void CContainer::LoadMassRenameStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_MASSRENAME_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptMassRename,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bMassRenameDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void CContainer::LoadMergeFilesStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_MERGEFILES_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptMergeFiles,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bMergeFilesDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void CContainer::LoadOrganizeBookmarksStateFromRegistry(HKEY hParentKey)
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

void CContainer::LoadSearchStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	SearchDirectoryInfo_t	sdi;
	SearchPatternInfo_t	spi;
	TCHAR				szItemKey[128];
	DWORD				dwSize;
	LONG				ReturnValue;
	int					i = 0;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_SEARCH_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptSearch,&dwSize);

		ReadDwordFromRegistry(hKey,_T("Width"),(DWORD *)&m_iSearchWidth);
		ReadDwordFromRegistry(hKey,_T("Height"),(DWORD *)&m_iSearchHeight);

		m_iColumnWidth1 = -1;
		m_iColumnWidth2 = -1;
		ReadDwordFromRegistry(hKey,_T("ColumnWidth1"),(DWORD *)&m_iColumnWidth1);
		ReadDwordFromRegistry(hKey,_T("ColumnWidth2"),(DWORD *)&m_iColumnWidth2);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bSearchDlgStateSaved = TRUE;
		}

		StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
			_T("Directory%d"),i);

		ReturnValue = ReadStringFromRegistry(hKey,szItemKey,
			sdi.szDirectory,SIZEOF_ARRAY(sdi.szDirectory));

		while(ReturnValue == ERROR_SUCCESS)
		{
			m_SearchDirectories.push_back(sdi);

			i++;

			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
				_T("Directory%d"),i);

			ReturnValue = ReadStringFromRegistry(hKey,szItemKey,
				sdi.szDirectory,SIZEOF_ARRAY(sdi.szDirectory));
		}

		i = 0;

		StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
			_T("Pattern%d"),i);

		ReturnValue = ReadStringFromRegistry(hKey,szItemKey,
			spi.szPattern,SIZEOF_ARRAY(spi.szPattern));

		while(ReturnValue == ERROR_SUCCESS)
		{
			m_SearchPatterns.push_back(spi);

			i++;

			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
				_T("Pattern%d"),i);

			ReturnValue = ReadStringFromRegistry(hKey,szItemKey,
				spi.szPattern,SIZEOF_ARRAY(spi.szPattern));
		}

		ReadStringFromRegistry(hKey,_T("SearchDirectoryText"),
			m_SearchPatternText,SIZEOF_ARRAY(m_SearchPatternText));

		ReadDwordFromRegistry(hKey,_T("SearchSubFolders"),(LPDWORD)&m_bSearchSubFolders);

		RegCloseKey(hKey);
	}
}

void CContainer::LoadSelectColumnsStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_SELECTCOLUMNS_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptSelectColumns,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bSelectColumnsDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void CContainer::LoadSelectDefaultColumnsStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_SELECTDEFAULTCOLUMNS_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptSetDefaultColumns,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bSetDefaultColumnsDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void CContainer::LoadSetFileAttributesStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_SETFILEATTRIBUTES_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptSetFileAttributes,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bSetFileAttributesDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void CContainer::LoadSplitFileStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	DWORD				dwSize;
	LONG				ReturnValue;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_SPLITFILE_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptSplitFile,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bSplitFileDlgStateSaved = TRUE;
		}

		RegCloseKey(hKey);
	}
}

void CContainer::LoadWildcardStateFromRegistry(HKEY hParentKey)
{
	HKEY				hKey;
	WildcardSelectInfo_t	wsi;
	TCHAR				szItemKey[128];
	DWORD				dwSize;
	LONG				ReturnValue;
	int					i = 0;

	ReturnValue = RegOpenKeyEx(hParentKey,REG_WILDCARD_KEY,0,
		KEY_READ,&hKey);

	if(ReturnValue == ERROR_SUCCESS)
	{
		dwSize = sizeof(POINT);
		ReturnValue = RegQueryValueEx(hKey,_T("Position"),
			NULL,NULL,(LPBYTE)&m_ptWildcardSelect,&dwSize);

		if(ReturnValue == ERROR_SUCCESS)
		{
			m_bWildcardDlgStateSaved = TRUE;
		}

		StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
			_T("Pattern%d"),i);

		ReturnValue = ReadStringFromRegistry(hKey,szItemKey,
			wsi.szPattern,SIZEOF_ARRAY(wsi.szPattern));

		while(ReturnValue == ERROR_SUCCESS)
		{
			m_wsiList.push_back(wsi);

			i++;

			StringCchPrintf(szItemKey,SIZEOF_ARRAY(szItemKey),
				_T("%d"),i);

			ReturnValue = ReadStringFromRegistry(hKey,szItemKey,
				wsi.szPattern,SIZEOF_ARRAY(wsi.szPattern));
		}

		ReadStringFromRegistry(hKey,_T("CurrentText"),
			m_szwsiText,SIZEOF_ARRAY(m_szwsiText));

		RegCloseKey(hKey);
	}
}

LONG CContainer::CLoadSaveRegistry::LoadWindowPosition(InitialWindowPos_t *piwp)
{
	HKEY			hKey;
	LONG			res;
	DWORD			Type;
	DWORD			SizeOfData;

	/* Create/Open the main registry key used to save the window position values. */
	res = RegOpenKeyEx(HKEY_CURRENT_USER,REG_WINDOWPOS_KEY,0,KEY_READ,&hKey);

	/* Only attempt to save the values if the main key was opened (or created properly). */
	if(res == ERROR_SUCCESS)
	{
		/* Must indicate to the registry key function the size of the data that
		is being retrieved. */
		SizeOfData = sizeof(LONG);

		/* Load the window position values... */
		RegQueryValueEx(hKey,_T("Left"),0,&Type,(LPBYTE)&piwp->rcNormalPosition.left,&SizeOfData);
		RegQueryValueEx(hKey,_T("Top"),0,&Type,(LPBYTE)&piwp->rcNormalPosition.top,&SizeOfData);
		RegQueryValueEx(hKey,_T("Right"),0,&Type,(LPBYTE)&piwp->rcNormalPosition.right,&SizeOfData);
		RegQueryValueEx(hKey,_T("Bottom"),0,&Type,(LPBYTE)&piwp->rcNormalPosition.bottom,&SizeOfData);

		SizeOfData = sizeof(BOOL);
		RegQueryValueEx(hKey,_T("Maximized"),0,&Type,(LPBYTE)&piwp->bMaximized,&SizeOfData);
	}

	/* Close the main key. */
	RegCloseKey(hKey);

	return res;
}

void CContainer::CLoadSaveRegistry::LoadGenericSettings(void)
{
	m_pContainer->LoadSettings(REG_MAIN_KEY);
}

void CContainer::CLoadSaveRegistry::LoadFilters(void)
{
	m_pContainer->LoadFilters();
}

void CContainer::CLoadSaveRegistry::LoadBookmarks(void)
{
	m_pContainer->LoadBookmarksFromRegistry();
}

int CContainer::CLoadSaveRegistry::LoadPreviousTabs(void)
{
	return m_pContainer->LoadTabSettingsFromRegistry();
}

void CContainer::CLoadSaveRegistry::LoadDefaultColumns(void)
{
	m_pContainer->LoadDefaultColumnsFromRegistry();
}

void CContainer::CLoadSaveRegistry::LoadApplicationToolbar(void)
{
	m_pContainer->LoadApplicationToolbarFromRegistry();
}

void CContainer::CLoadSaveRegistry::LoadToolbarInformation(void)
{
	m_pContainer->LoadToolbarInformationFromRegistry();
}

void CContainer::CLoadSaveRegistry::LoadColorRules(void)
{
	m_pContainer->LoadColorRulesFromRegistry();
}

void CContainer::CLoadSaveRegistry::LoadState(void)
{
	m_pContainer->LoadStateFromRegistry();
}

void CContainer::CLoadSaveRegistry::SaveGenericSettings(void)
{
	m_pContainer->SaveSettings();
}

LONG CContainer::CLoadSaveRegistry::SaveWindowPosition(void)
{
	::SaveWindowPosition(m_pContainer->m_hContainer,REG_WINDOWPOS_KEY);
	return 0;
}

void CContainer::CLoadSaveRegistry::SaveFilters(void)
{
	m_pContainer->SaveFilters();
}

void CContainer::CLoadSaveRegistry::SaveBookmarks(void)
{
	m_pContainer->SaveBookmarksToRegistry();
}

void CContainer::CLoadSaveRegistry::SaveTabs(void)
{
	m_pContainer->SaveTabSettingsToRegistry();
}

void CContainer::CLoadSaveRegistry::SaveDefaultColumns(void)
{
	m_pContainer->SaveDefaultColumnsToRegistry();
}

void CContainer::CLoadSaveRegistry::SaveApplicationToolbar(void)
{
	m_pContainer->SaveApplicationToolbarToRegistry();
}

void CContainer::CLoadSaveRegistry::SaveToolbarInformation(void)
{
	m_pContainer->SaveToolbarInformationToRegistry();
}

void CContainer::CLoadSaveRegistry::SaveColorRules(void)
{
	m_pContainer->SaveColorRulesToRegistry();
}

void CContainer::CLoadSaveRegistry::SaveState(void)
{
	m_pContainer->SaveStateToRegistry();
}