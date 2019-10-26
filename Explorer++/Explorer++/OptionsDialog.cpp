// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Notes:
 *  - Center dialog. Don't remember previous position.
 *  - The apply button is called for each dialog that
 *    has been initialised (i.e. any dialog that has
 *    had the focus set to itself).
 */

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "MainImages.h"
#include "MainResource.h"
#include "ModelessDialogs.h"
#include "SetDefaultColumnsDialog.h"
#include "ShellBrowser/ViewModes.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include <boost/range/adaptor/map.hpp>

#define NUM_DIALOG_OPTIONS_PAGES	5
#define LANG_SINHALA				1115

INT_PTR CALLBACK	FilesFoldersProcStub(HWND,UINT,WPARAM,LPARAM);
INT_PTR CALLBACK	WindowProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	GeneralSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	DefaultSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK	TabSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

int CALLBACK PropSheetProcStub(HWND hDlg,UINT msg,LPARAM lParam);
int CALLBACK NewTabDirectoryBrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData);

struct FileSize_t
{
	SizeDisplayFormat_t sdf;
	UINT StringID;
};

static const FileSize_t FILE_SIZES[] =
{{SIZE_FORMAT_BYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_BYTES},
{SIZE_FORMAT_KBYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_KB},
{SIZE_FORMAT_MBYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_MB},
{SIZE_FORMAT_GBYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_GB},
{SIZE_FORMAT_TBYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_TB},
{SIZE_FORMAT_PBYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_PB}};

static HWND g_hOptionsPropertyDialog	= NULL;

HICON g_hNewTabDirIcon;
TCHAR g_szNewTabDirectory[MAX_PATH];

void Explorerplusplus::ShowOptions(void)
{
	PROPSHEETPAGE	psp[NUM_DIALOG_OPTIONS_PAGES];
	HPROPSHEETPAGE	hpsp[NUM_DIALOG_OPTIONS_PAGES];
	PROPSHEETHEADER	psh;
	HIMAGELIST		himl;
	HBITMAP			hBitmap;
	TCHAR			szTitle[64];
	unsigned int	nSheet = 0;

	himl = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);

	/* Contains all images used on the menus. */
	hBitmap = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));

	ImageList_Add(himl,hBitmap,NULL);

	g_hNewTabDirIcon = ImageList_GetIcon(himl,SHELLIMAGES_NEWTAB,ILD_NORMAL);

	/* General options page. */
	psp[nSheet].dwSize		= sizeof(PROPSHEETPAGE);
	psp[nSheet].dwFlags		= PSP_DEFAULT;
	psp[nSheet].hInstance	= m_hLanguageModule;
	psp[nSheet].pszTemplate	= MAKEINTRESOURCE(IDD_OPTIONS_GENERAL);
	psp[nSheet].lParam		= (LPARAM)this;
	psp[nSheet].pfnDlgProc	= GeneralSettingsProcStub;

	hpsp[nSheet] = CreatePropertySheetPage(&psp[nSheet]);
	nSheet++;

	/* Files and Folders options page. */
	psp[nSheet].dwSize		= sizeof(PROPSHEETPAGE);
	psp[nSheet].dwFlags		= PSP_DEFAULT;
	psp[nSheet].hInstance	= m_hLanguageModule;
	psp[nSheet].pszTemplate	= MAKEINTRESOURCE(IDD_OPTIONS_FILESFOLDERS);
	psp[nSheet].lParam		= (LPARAM)this;
	psp[nSheet].pfnDlgProc	= FilesFoldersProcStub;

	hpsp[nSheet] = CreatePropertySheetPage(&psp[nSheet]);
	nSheet++;

	/* Window options page. */
	psp[nSheet].dwSize		= sizeof(PROPSHEETPAGE);
	psp[nSheet].dwFlags		= PSP_DEFAULT;
	psp[nSheet].hInstance	= m_hLanguageModule;
	psp[nSheet].pszTemplate	= MAKEINTRESOURCE(IDD_OPTIONS_WINDOW);
	psp[nSheet].lParam		= (LPARAM)this;
	psp[nSheet].pfnDlgProc	= WindowProcStub;

	hpsp[nSheet] = CreatePropertySheetPage(&psp[nSheet]);
	nSheet++;

	/* Tab settings options page. */
	psp[nSheet].dwSize		= sizeof(PROPSHEETPAGE);
	psp[nSheet].dwFlags		= PSP_DEFAULT;
	psp[nSheet].hInstance	= m_hLanguageModule;
	psp[nSheet].pszTemplate	= MAKEINTRESOURCE(IDD_OPTIONS_TABS);
	psp[nSheet].lParam		= (LPARAM)this;
	psp[nSheet].pfnDlgProc	= TabSettingsProcStub;

	hpsp[nSheet] = CreatePropertySheetPage(&psp[nSheet]);
	nSheet++;

	/* Default settings options page. */
	psp[nSheet].dwSize		= sizeof(PROPSHEETPAGE);
	psp[nSheet].dwFlags		= PSP_DEFAULT;
	psp[nSheet].hInstance	= m_hLanguageModule;
	psp[nSheet].pszTemplate	= MAKEINTRESOURCE(IDD_OPTIONS_DEFAULT);
	psp[nSheet].lParam		= (LPARAM)this;
	psp[nSheet].pfnDlgProc	= DefaultSettingsProcStub;

	hpsp[nSheet] = CreatePropertySheetPage(&psp[nSheet]);
	nSheet++;

	/* Load the main dialog title. */
	LoadString(m_hLanguageModule,IDS_OPTIONSDIALOG_TITLE,
		szTitle,SIZEOF_ARRAY(szTitle));

	psh.dwSize		= sizeof(PROPSHEETHEADER);
	psh.dwFlags		= PSH_DEFAULT|PSH_USECALLBACK|PSH_NOCONTEXTHELP|PSH_USEHICON|PSH_MODELESS;
	psh.hwndParent	= m_hContainer;
	psh.pszCaption	= szTitle;
	psh.nPages		= nSheet;
	psh.nStartPage	= 0;

	psh.hIcon		= ImageList_GetIcon(himl,SHELLIMAGES_OPTIONS,ILD_TRANSPARENT);

	psh.ppsp		= psp;
	psh.phpage		= hpsp;
	psh.pfnCallback	= PropSheetProcStub;

	DeleteObject(hBitmap);
	ImageList_Destroy(himl);

	/* Create the property dialog itself, which
	will hold each of the above property pages. */
	g_hwndOptions = (HWND)PropertySheet(&psh);
}

int CALLBACK PropSheetProcStub(HWND hDlg,UINT msg,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch(msg)
	{
		case PSCB_INITIALIZED:
			g_hOptionsPropertyDialog = hDlg;
			break;
	}

	return 0;
}

INT_PTR CALLBACK GeneralSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				PROPSHEETPAGE *ppsp;

				ppsp = (PROPSHEETPAGE *)lParam;
				pContainer = (Explorerplusplus *)ppsp->lParam;
			}
			break;
	}

	return pContainer->GeneralSettingsProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::GeneralSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				HWND hButton;
				HWND hEdit;
				int nIDButton;

				switch(m_config->startupMode)
				{
				case STARTUP_PREVIOUSTABS:
					nIDButton = IDC_STARTUP_PREVIOUSTABS;
					break;

				case STARTUP_DEFAULTFOLDER:
					nIDButton = IDC_STARTUP_DEFAULTFOLDER;
					break;

				default:
					nIDButton = IDC_STARTUP_PREVIOUSTABS;
					m_config->startupMode = STARTUP_PREVIOUSTABS;
					break;
				}
				CheckDlgButton(hDlg,nIDButton,BST_CHECKED);

				switch(m_config->replaceExplorerMode)
				{
				case NDefaultFileManager::REPLACEEXPLORER_NONE:
					nIDButton = IDC_OPTION_REPLACEEXPLORER_NONE;
					break;

				case NDefaultFileManager::REPLACEEXPLORER_FILESYSTEM:
					nIDButton = IDC_OPTION_REPLACEEXPLORER_FILESYSTEM;
					break;

				case NDefaultFileManager::REPLACEEXPLORER_ALL:
					nIDButton = IDC_OPTION_REPLACEEXPLORER_ALL;
					break;

				default:
					nIDButton = IDC_OPTION_REPLACEEXPLORER_NONE;
					break;
				}
				CheckDlgButton(hDlg,nIDButton,BST_CHECKED);

				if(m_bSavePreferencesToXMLFile)
					CheckDlgButton(hDlg,IDC_OPTION_XML,BST_CHECKED);

				hButton = GetDlgItem(hDlg,IDC_DEFAULT_NEWTABDIR_BUTTON);
				SendMessage(hButton,BM_SETIMAGE,IMAGE_ICON,(LPARAM)g_hNewTabDirIcon);

				hEdit = GetDlgItem(hDlg,IDC_DEFAULT_NEWTABDIR_EDIT);
				DefaultSettingsSetNewTabDir(hEdit,m_config->defaultTabDirectory.c_str());

				AddLanguages(hDlg);

				CenterWindow(m_hContainer,g_hOptionsPropertyDialog);
			}
			break;

		case WM_COMMAND:
			if(HIWORD(wParam) != 0)
			{
				switch(HIWORD(wParam))
				{
				case EN_CHANGE:
				case CBN_SELCHANGE:
					PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;
				}
			}
			else
			{
				switch(LOWORD(wParam))
				{
				case IDC_STARTUP_PREVIOUSTABS:
				case IDC_STARTUP_DEFAULTFOLDER:
					if(IsDlgButtonChecked(hDlg,LOWORD(wParam)) == BST_CHECKED)
						PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;

				case IDC_OPTION_REPLACEEXPLORER_NONE:
				case IDC_OPTION_REPLACEEXPLORER_FILESYSTEM:
				case IDC_OPTION_REPLACEEXPLORER_ALL:
				case IDC_OPTION_XML:
					PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;

				case IDC_DEFAULT_NEWTABDIR_BUTTON:
					OnDefaultSettingsNewTabDir(hDlg);
					PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;
				}
			}
			break;

		case WM_NOTIFY:
		{
			NMHDR	*nmhdr = NULL;
			nmhdr = (NMHDR *)lParam;

			switch(nmhdr->code)
			{
				case PSN_APPLY:
					{
						HWND hEdit;
						TCHAR szNewTabDir[MAX_PATH];
						TCHAR szVirtualParsingPath[MAX_PATH];
						NDefaultFileManager::ReplaceExplorerModes_t ReplaceExplorerMode = NDefaultFileManager::REPLACEEXPLORER_NONE;
						BOOL bSuccess;
						HRESULT hr;
						int iSel;

						if(IsDlgButtonChecked(hDlg,IDC_STARTUP_PREVIOUSTABS) == BST_CHECKED)
							m_config->startupMode = STARTUP_PREVIOUSTABS;
						else if(IsDlgButtonChecked(hDlg,IDC_STARTUP_DEFAULTFOLDER) == BST_CHECKED)
							m_config->startupMode = STARTUP_DEFAULTFOLDER;

						if(IsDlgButtonChecked(hDlg,IDC_OPTION_REPLACEEXPLORER_NONE) == BST_CHECKED)
							ReplaceExplorerMode = NDefaultFileManager::REPLACEEXPLORER_NONE;
						else if(IsDlgButtonChecked(hDlg,IDC_OPTION_REPLACEEXPLORER_FILESYSTEM) == BST_CHECKED)
							ReplaceExplorerMode = NDefaultFileManager::REPLACEEXPLORER_FILESYSTEM;
						else if(IsDlgButtonChecked(hDlg,IDC_OPTION_REPLACEEXPLORER_ALL) == BST_CHECKED)
							ReplaceExplorerMode = NDefaultFileManager::REPLACEEXPLORER_ALL;

						if(m_config->replaceExplorerMode != ReplaceExplorerMode)
						{
							bSuccess = TRUE;

							TCHAR menuText[256];
							LoadString(m_hLanguageModule, IDS_OPEN_IN_EXPLORERPLUSPLUS,
								menuText, SIZEOF_ARRAY(menuText));

							switch(ReplaceExplorerMode)
							{
							case NDefaultFileManager::REPLACEEXPLORER_NONE:
								{
									switch(m_config->replaceExplorerMode)
									{
									case NDefaultFileManager::REPLACEEXPLORER_FILESYSTEM:
										bSuccess = NDefaultFileManager::RemoveAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
										break;

									case NDefaultFileManager::REPLACEEXPLORER_ALL:
										bSuccess = NDefaultFileManager::RemoveAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
										break;

									default:
										bSuccess = TRUE;
										break;
									}
								}
								break;

							case NDefaultFileManager::REPLACEEXPLORER_FILESYSTEM:
								NDefaultFileManager::RemoveAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
								NDefaultFileManager::RemoveAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
								bSuccess = NDefaultFileManager::SetAsDefaultFileManagerFileSystem(
									SHELL_DEFAULT_INTERNAL_COMMAND_NAME, menuText);
								break;

							case NDefaultFileManager::REPLACEEXPLORER_ALL:
								NDefaultFileManager::RemoveAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
								NDefaultFileManager::RemoveAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
								bSuccess = NDefaultFileManager::SetAsDefaultFileManagerAll(
									SHELL_DEFAULT_INTERNAL_COMMAND_NAME, menuText);
								break;
							}

							if(bSuccess)
							{
								m_config->replaceExplorerMode = ReplaceExplorerMode;
							}
							else
							{
								TCHAR szErrorMsg[256];
								LoadString(m_hLanguageModule,IDS_ERR_FILEMANAGERSETTING,
									szErrorMsg,SIZEOF_ARRAY(szErrorMsg));
								MessageBox(hDlg,szErrorMsg,NExplorerplusplus::APP_NAME,MB_ICONWARNING);

								int nIDButton;

								switch(ReplaceExplorerMode)
								{
								case NDefaultFileManager::REPLACEEXPLORER_NONE:
									nIDButton = IDC_OPTION_REPLACEEXPLORER_NONE;
									break;

								case NDefaultFileManager::REPLACEEXPLORER_FILESYSTEM:
									nIDButton = IDC_OPTION_REPLACEEXPLORER_FILESYSTEM;
									break;

								case NDefaultFileManager::REPLACEEXPLORER_ALL:
									nIDButton = IDC_OPTION_REPLACEEXPLORER_ALL;
									break;

								default:
									nIDButton = IDC_OPTION_REPLACEEXPLORER_NONE;
									break;
								}
								CheckDlgButton(hDlg,nIDButton,BST_UNCHECKED);

								/* The default file manager setting was not changed, so
								reset the state of the file manager radio buttons. */
								switch(m_config->replaceExplorerMode)
								{
								case NDefaultFileManager::REPLACEEXPLORER_NONE:
									nIDButton = IDC_OPTION_REPLACEEXPLORER_NONE;
									break;

								case NDefaultFileManager::REPLACEEXPLORER_FILESYSTEM:
									nIDButton = IDC_OPTION_REPLACEEXPLORER_FILESYSTEM;
									break;

								case NDefaultFileManager::REPLACEEXPLORER_ALL:
									nIDButton = IDC_OPTION_REPLACEEXPLORER_ALL;
									break;

								default:
									nIDButton = IDC_OPTION_REPLACEEXPLORER_NONE;
									break;
								}
								CheckDlgButton(hDlg,nIDButton,BST_CHECKED);
							}
						}

						m_bSavePreferencesToXMLFile = (IsDlgButtonChecked(hDlg,IDC_OPTION_XML)
							== BST_CHECKED);

						hEdit = GetDlgItem(hDlg,IDC_DEFAULT_NEWTABDIR_EDIT);

						SendMessage(hEdit,WM_GETTEXT,SIZEOF_ARRAY(szNewTabDir),
							(LPARAM)szNewTabDir);

						/* The folder may be virtual, in which case, it needs
						to be decoded. */
						hr = DecodeFriendlyPath(szNewTabDir,szVirtualParsingPath,SIZEOF_ARRAY(szVirtualParsingPath));

						if (SUCCEEDED(hr))
						{
							m_config->defaultTabDirectory = szVirtualParsingPath;
						}
						else
						{
							m_config->defaultTabDirectory = szNewTabDir;
						}

						iSel = (int)SendMessage(GetDlgItem(hDlg,IDC_OPTIONS_LANGUAGE),CB_GETCURSEL,0,0);

						m_Language = GetLanguageIDFromIndex(hDlg,iSel);

						SaveAllSettings();
					}
					break;
			}
		}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;

		case WM_DESTROY:
			DestroyIcon(g_hNewTabDirIcon);
			break;
	}

	return 0;
}

INT_PTR CALLBACK FilesFoldersProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				PROPSHEETPAGE *ppsp;

				ppsp = (PROPSHEETPAGE *)lParam;
				pContainer = (Explorerplusplus *)ppsp->lParam;
			}
			break;
	}

	return pContainer->FilesFoldersProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::FilesFoldersProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				HWND hCBSize;

				if(m_config->globalFolderSettings.hideSystemFiles)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_SYSTEMFILES,BST_CHECKED);
				if(!m_config->globalFolderSettings.showExtensions)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_EXTENSIONS,BST_CHECKED);
				if(m_config->globalFolderSettings.hideLinkExtension)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_LINK,BST_CHECKED);
				if(m_config->globalFolderSettings.insertSorted)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_INSERTSORTED,BST_CHECKED);
				if(m_config->globalFolderSettings.oneClickActivate)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_SINGLECLICK,BST_CHECKED);
				
				SetDlgItemInt(hDlg,IDC_OPTIONS_HOVER_TIME,m_config->globalFolderSettings.oneClickActivateHoverTime,FALSE);
				EnableWindow(GetDlgItem(hDlg,IDC_OPTIONS_HOVER_TIME),m_config->globalFolderSettings.oneClickActivate);
				EnableWindow(GetDlgItem(hDlg,IDC_LABEL_HOVER_TIME),m_config->globalFolderSettings.oneClickActivate);

				if(m_config->overwriteExistingFilesConfirmation)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION,BST_CHECKED);
				if(m_config->playNavigationSound)
					CheckDlgButton(hDlg,IDC_OPTIONS_PLAYNAVIGATIONSOUND,BST_CHECKED);
				if(m_config->globalFolderSettings.showFolderSizes)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_FOLDERSIZES,BST_CHECKED);
				if(m_config->globalFolderSettings.disableFolderSizesNetworkRemovable)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE,BST_CHECKED);
				if(m_config->globalFolderSettings.forceSize)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_FORCESIZE,BST_CHECKED);
				if(m_config->handleZipFiles)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_ZIPFILES,BST_CHECKED);
				if(m_config->globalFolderSettings.showFriendlyDates)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_FRIENDLYDATES,BST_CHECKED);
				if(m_config->showInfoTips)
					CheckDlgButton(hDlg,IDC_OPTIONS_CHECK_SHOWINFOTIPS,BST_CHECKED);

				if(m_config->infoTipType == INFOTIP_SYSTEM)
					CheckDlgButton(hDlg,IDC_OPTIONS_RADIO_SYSTEMINFOTIPS,BST_CHECKED);
				else
					CheckDlgButton(hDlg,IDC_OPTIONS_RADIO_CUSTOMINFOTIPS,BST_CHECKED);

				hCBSize = GetDlgItem(hDlg,IDC_COMBO_FILESIZES);

				for(int i = 0;i < SIZEOF_ARRAY(FILE_SIZES);i++)
				{
					TCHAR szTemp[32];
					LoadString(m_hLanguageModule,FILE_SIZES[i].StringID,szTemp,SIZEOF_ARRAY(szTemp));
					SendMessage(hCBSize,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(szTemp));
					SendMessage(hCBSize,CB_SETITEMDATA,i,FILE_SIZES[i].sdf);

					if(FILE_SIZES[i].sdf == m_config->globalFolderSettings.sizeDisplayFormat)
					{
						SendMessage(hCBSize,CB_SETCURSEL,i,0);
					}
				}

				EnableWindow(hCBSize,m_config->globalFolderSettings.forceSize);

				SetInfoTipWindowStates(hDlg);
				SetFolderSizeWindowState(hDlg);
			}
			break;

		case WM_COMMAND:
			if(HIWORD(wParam) != 0)
			{
				switch(HIWORD(wParam))
				{
				case CBN_SELCHANGE:
					PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;
				}
			}
			else
			{
				switch(LOWORD(wParam))
				{
				case IDC_SETTINGS_CHECK_SYSTEMFILES:
				case IDC_SETTINGS_CHECK_EXTENSIONS:
				case IDC_SETTINGS_CHECK_LINK:
				case IDC_SETTINGS_CHECK_INSERTSORTED:
				case IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION:
				case IDC_OPTIONS_PLAYNAVIGATIONSOUND:
				case IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE:
				case IDC_SETTINGS_CHECK_ZIPFILES:
				case IDC_SETTINGS_CHECK_FRIENDLYDATES:
				case IDC_OPTIONS_HOVER_TIME:
					PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;

				case IDC_SETTINGS_CHECK_FORCESIZE:
					EnableWindow(GetDlgItem(hDlg,IDC_COMBO_FILESIZES),IsDlgButtonChecked(hDlg,LOWORD(wParam)) == BST_CHECKED);
					PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;

				case IDC_OPTIONS_RADIO_SYSTEMINFOTIPS:
				case IDC_OPTIONS_RADIO_CUSTOMINFOTIPS:
					if(IsDlgButtonChecked(hDlg,LOWORD(wParam)) == BST_CHECKED)
						PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;

				case IDC_OPTIONS_CHECK_SHOWINFOTIPS:
					SetInfoTipWindowStates(hDlg);
					PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;

				case IDC_SETTINGS_CHECK_FOLDERSIZES:
					SetFolderSizeWindowState(hDlg);
					PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;

				case IDC_SETTINGS_CHECK_SINGLECLICK:
					EnableWindow(GetDlgItem(hDlg,IDC_OPTIONS_HOVER_TIME),IsDlgButtonChecked(hDlg,LOWORD(wParam)) == BST_CHECKED);
					EnableWindow(GetDlgItem(hDlg,IDC_LABEL_HOVER_TIME),IsDlgButtonChecked(hDlg,LOWORD(wParam)) == BST_CHECKED);
					PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;
				}
			}
			break;

		case WM_NOTIFY:
			{
				NMHDR	*nmhdr = NULL;
				nmhdr = (NMHDR *)lParam;

				switch(nmhdr->code)
				{
				case PSN_APPLY:
					{
						HWND hCBSize;
						int iSel;

						m_config->globalFolderSettings.hideSystemFiles = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_SYSTEMFILES)
							== BST_CHECKED);

						m_config->globalFolderSettings.showExtensions = !(IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_EXTENSIONS)
							== BST_CHECKED);

						m_config->globalFolderSettings.hideLinkExtension = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_LINK)
							== BST_CHECKED);

						m_config->globalFolderSettings.insertSorted = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_INSERTSORTED)
							== BST_CHECKED);

						m_config->globalFolderSettings.oneClickActivate = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_SINGLECLICK)
							== BST_CHECKED);

						m_config->globalFolderSettings.oneClickActivateHoverTime = GetDlgItemInt(hDlg,IDC_OPTIONS_HOVER_TIME,NULL,FALSE);

						m_config->overwriteExistingFilesConfirmation = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION)
							== BST_CHECKED);

						m_config->playNavigationSound = (IsDlgButtonChecked(hDlg,IDC_OPTIONS_PLAYNAVIGATIONSOUND)
							== BST_CHECKED);

						m_config->globalFolderSettings.showFolderSizes = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_FOLDERSIZES)
							== BST_CHECKED);

						m_config->globalFolderSettings.disableFolderSizesNetworkRemovable = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE)
							== BST_CHECKED);

						m_config->globalFolderSettings.forceSize = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_FORCESIZE)
							== BST_CHECKED);

						m_config->handleZipFiles = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_ZIPFILES)
							== BST_CHECKED);

						m_config->globalFolderSettings.showFriendlyDates = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_FRIENDLYDATES)
							== BST_CHECKED);

						m_config->showInfoTips = (IsDlgButtonChecked(hDlg,IDC_OPTIONS_CHECK_SHOWINFOTIPS)
							== BST_CHECKED);

						if(IsDlgButtonChecked(hDlg,IDC_OPTIONS_RADIO_SYSTEMINFOTIPS) == BST_CHECKED)
							m_config->infoTipType = INFOTIP_SYSTEM;
						else
							m_config->infoTipType = INFOTIP_CUSTOM;

						hCBSize = GetDlgItem(hDlg,IDC_COMBO_FILESIZES);

						iSel = (int)SendMessage(hCBSize,CB_GETCURSEL,0,0);
						m_config->globalFolderSettings.sizeDisplayFormat = (SizeDisplayFormat_t)SendMessage(hCBSize,CB_GETITEMDATA,iSel,0);

						for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
						{
							RefreshTab(tab);

							NListView::ListView_ActivateOneClickSelect(tab.listView, m_config->globalFolderSettings.oneClickActivate,
								m_config->globalFolderSettings.oneClickActivateHoverTime);
						}

						SaveAllSettings();
					}
					break;
				}
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

INT_PTR CALLBACK WindowProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				PROPSHEETPAGE *ppsp;

				ppsp = (PROPSHEETPAGE *)lParam;
				pContainer = (Explorerplusplus *)ppsp->lParam;
			}
			break;
	}

	return pContainer->WindowProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::WindowProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			if(m_config->allowMultipleInstances)
				CheckDlgButton(hDlg,IDC_OPTION_MULTIPLEINSTANCES,BST_CHECKED);
			if(m_config->useLargeToolbarIcons)
				CheckDlgButton(hDlg,IDC_OPTION_LARGETOOLBARICONS,BST_CHECKED);
			if(m_config->alwaysShowTabBar.get())
				CheckDlgButton(hDlg,IDC_OPTION_ALWAYSSHOWTABBAR,BST_CHECKED);
			if(m_config->showTabBarAtBottom)
				CheckDlgButton(hDlg,IDC_OPTION_SHOWTABBARATBOTTOM,BST_CHECKED);
			if(m_config->showFilePreviews)
				CheckDlgButton(hDlg,IDC_OPTION_FILEPREVIEWS,BST_CHECKED);
			if(m_config->showFullTitlePath.get())
				CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_TITLEPATH,BST_CHECKED);
			if(m_config->showUserNameInTitleBar.get())
				CheckDlgButton(hDlg,IDC_OPTION_USERNAMEINTITLEBAR,BST_CHECKED);
			if(m_config->showPrivilegeLevelInTitleBar.get())
				CheckDlgButton(hDlg,IDC_OPTION_PRIVILEGELEVELINTITLEBAR,BST_CHECKED);
			if(m_config->synchronizeTreeview)
				CheckDlgButton(hDlg,IDC_OPTION_SYNCTREEVIEW,BST_CHECKED);
			if(m_config->treeViewAutoExpandSelected)
				CheckDlgButton(hDlg,IDC_OPTION_TREEVIEWSELECTIONEXPAND,BST_CHECKED);
			if(!m_config->treeViewDelayEnabled)
				CheckDlgButton(hDlg,IDC_OPTION_TREEVIEWDELAY,BST_CHECKED);
			if(m_config->extendTabControl)
				CheckDlgButton(hDlg,IDC_OPTION_EXTENDTABCONTROL,BST_CHECKED);
			if(m_config->globalFolderSettings.showGridlines)
				CheckDlgButton(hDlg,IDC_OPTION_GRIDLINES,BST_CHECKED);
			if(m_config->checkBoxSelection)
				CheckDlgButton(hDlg,IDC_OPTION_CHECKBOXSELECTION,BST_CHECKED);
			if(m_config->useFullRowSelect)
				CheckDlgButton(hDlg,IDC_OPTION_FULLROWSELECT,BST_CHECKED);
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_OPTION_MULTIPLEINSTANCES:
		case IDC_OPTION_LARGETOOLBARICONS:
		case IDC_OPTION_ALWAYSSHOWTABBAR:
		case IDC_OPTION_SHOWTABBARATBOTTOM:
		case IDC_OPTION_FILEPREVIEWS:
		case IDC_SETTINGS_CHECK_TITLEPATH:
		case IDC_OPTION_USERNAMEINTITLEBAR:
		case IDC_OPTION_PRIVILEGELEVELINTITLEBAR:
		case IDC_OPTION_SYNCTREEVIEW:
		case IDC_OPTION_TREEVIEWSELECTIONEXPAND:
		case IDC_OPTION_TREEVIEWDELAY:
		case IDC_OPTION_EXTENDTABCONTROL:
		case IDC_OPTION_GRIDLINES:
		case IDC_OPTION_CHECKBOXSELECTION:
		case IDC_OPTION_FULLROWSELECT:
			PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
			break;
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR	*nmhdr = NULL;
			nmhdr = (NMHDR *)lParam;

			switch(nmhdr->code)
			{
			case PSN_APPLY:
				{
					BOOL bCheckBoxSelection;

					m_config->allowMultipleInstances = (IsDlgButtonChecked(hDlg,IDC_OPTION_MULTIPLEINSTANCES)
						== BST_CHECKED);

					m_config->alwaysShowTabBar.set(IsDlgButtonChecked(hDlg,IDC_OPTION_ALWAYSSHOWTABBAR)
						== BST_CHECKED);

					m_config->showTabBarAtBottom = (IsDlgButtonChecked(hDlg,IDC_OPTION_SHOWTABBARATBOTTOM)
						== BST_CHECKED);

					m_config->showFilePreviews = (IsDlgButtonChecked(hDlg,IDC_OPTION_FILEPREVIEWS)
						== BST_CHECKED);

					m_config->showFullTitlePath.set(IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_TITLEPATH)
						== BST_CHECKED);

					m_config->showUserNameInTitleBar.set(IsDlgButtonChecked(hDlg,IDC_OPTION_USERNAMEINTITLEBAR)
						== BST_CHECKED);

					m_config->showPrivilegeLevelInTitleBar.set(IsDlgButtonChecked(hDlg,IDC_OPTION_PRIVILEGELEVELINTITLEBAR)
						== BST_CHECKED);

					m_config->synchronizeTreeview = (IsDlgButtonChecked(hDlg,IDC_OPTION_SYNCTREEVIEW)
						== BST_CHECKED);

					m_config->treeViewAutoExpandSelected = (IsDlgButtonChecked(hDlg,IDC_OPTION_TREEVIEWSELECTIONEXPAND)
						== BST_CHECKED);

					m_config->treeViewDelayEnabled = !(IsDlgButtonChecked(hDlg,IDC_OPTION_TREEVIEWDELAY)
						== BST_CHECKED);

					m_config->extendTabControl = (IsDlgButtonChecked(hDlg,IDC_OPTION_EXTENDTABCONTROL)
						== BST_CHECKED);

					m_config->globalFolderSettings.showGridlines = (IsDlgButtonChecked(hDlg,IDC_OPTION_GRIDLINES)
						== BST_CHECKED);

					bCheckBoxSelection = (IsDlgButtonChecked(hDlg,IDC_OPTION_CHECKBOXSELECTION)
						== BST_CHECKED);

					if(m_config->checkBoxSelection != bCheckBoxSelection)
					{
						for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
						{
							DWORD dwExtendedStyle = ListView_GetExtendedListViewStyle(tab.listView);

							if(bCheckBoxSelection)
							{
								dwExtendedStyle |= LVS_EX_CHECKBOXES;
							}
							else
							{
								dwExtendedStyle &= ~LVS_EX_CHECKBOXES;
							}

							ListView_SetExtendedListViewStyle(tab.listView, dwExtendedStyle);
						}

						m_config->checkBoxSelection = (IsDlgButtonChecked(hDlg,IDC_OPTION_CHECKBOXSELECTION)
							== BST_CHECKED);
					}

					m_config->useFullRowSelect = (IsDlgButtonChecked(hDlg,IDC_OPTION_FULLROWSELECT)
						== BST_CHECKED);

					BOOL bLargeToolbarIcons = (IsDlgButtonChecked(hDlg,IDC_OPTION_LARGETOOLBARICONS)
						== BST_CHECKED);

					if(m_config->useLargeToolbarIcons != bLargeToolbarIcons)
					{
						m_config->useLargeToolbarIcons = (IsDlgButtonChecked(hDlg,IDC_OPTION_LARGETOOLBARICONS)
							== BST_CHECKED);

						AdjustMainToolbarSize();
					}

					for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
					{
						/* TODO: The tab should monitor for settings
						changes itself. */
						tab.GetShellBrowser()->OnGridlinesSettingChanged();

						NListView::ListView_AddRemoveExtendedStyle(tab.listView,
							LVS_EX_FULLROWSELECT,m_config->useFullRowSelect);
					}

					SaveAllSettings();
				}
				break;
			}
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg,0);
		break;
	}

	return 0;
}

INT_PTR CALLBACK TabSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				PROPSHEETPAGE *ppsp;

				ppsp = (PROPSHEETPAGE *)lParam;
				pContainer = (Explorerplusplus *)ppsp->lParam;
			}
			break;
	}

	return pContainer->TabSettingsProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::TabSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				if(m_config->showTaskbarThumbnails)
					CheckDlgButton(hDlg,IDC_TABS_TASKBARTHUMBNAILS,BST_CHECKED);
				if(m_config->forceSameTabWidth.get())
					CheckDlgButton(hDlg,IDC_TABS_SAMEWIDTH,BST_CHECKED);
				if(m_config->confirmCloseTabs)
					CheckDlgButton(hDlg,IDC_TABS_CLOSECONFIRMATION,BST_CHECKED);
				if(m_config->openNewTabNextToCurrent)
					CheckDlgButton(hDlg,IDC_TABS_OPENNEXTTOCURRENT,BST_CHECKED);
				if(m_config->alwaysOpenNewTab)
					CheckDlgButton(hDlg,IDC_SETTINGS_CHECK_ALWAYSNEWTAB,BST_CHECKED);
				if(m_config->doubleClickTabClose)
					CheckDlgButton(hDlg,IDC_TABS_DOUBLECLICKCLOSE,BST_CHECKED);
				if(m_config->closeMainWindowOnTabClose)
					CheckDlgButton(hDlg,IDC_TABS_CLOSEMAINWINDOW,BST_CHECKED);
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDC_TABS_TASKBARTHUMBNAILS:
			case IDC_TABS_SAMEWIDTH:
			case IDC_TABS_CLOSECONFIRMATION:
			case IDC_TABS_OPENNEXTTOCURRENT:
			case IDC_SETTINGS_CHECK_ALWAYSNEWTAB:
			case IDC_TABS_DOUBLECLICKCLOSE:
			case IDC_TABS_CLOSEMAINWINDOW:
				PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
				break;
			}
			break;

		case WM_NOTIFY:
			{
				NMHDR	*nmhdr = NULL;
				nmhdr = (NMHDR *)lParam;

				switch(nmhdr->code)
				{
				case PSN_APPLY:
					{
						m_config->showTaskbarThumbnails = (IsDlgButtonChecked(hDlg,IDC_TABS_TASKBARTHUMBNAILS)
							== BST_CHECKED);

						m_config->forceSameTabWidth.set(IsDlgButtonChecked(hDlg,IDC_TABS_SAMEWIDTH)
							== BST_CHECKED);

						m_config->confirmCloseTabs = (IsDlgButtonChecked(hDlg,IDC_TABS_CLOSECONFIRMATION)
							== BST_CHECKED);

						m_config->openNewTabNextToCurrent = (IsDlgButtonChecked(hDlg,IDC_TABS_OPENNEXTTOCURRENT)
							== BST_CHECKED);

						m_config->alwaysOpenNewTab = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_ALWAYSNEWTAB)
							== BST_CHECKED);

						m_config->doubleClickTabClose = (IsDlgButtonChecked(hDlg,IDC_TABS_DOUBLECLICKCLOSE)
							== BST_CHECKED);

						m_config->closeMainWindowOnTabClose = (IsDlgButtonChecked(hDlg,IDC_TABS_CLOSEMAINWINDOW)
							== BST_CHECKED);

						SaveAllSettings();
					}
					break;
				}
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

INT_PTR CALLBACK DefaultSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static Explorerplusplus *pContainer;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				PROPSHEETPAGE *ppsp;

				ppsp = (PROPSHEETPAGE *)lParam;
				pContainer = (Explorerplusplus *)ppsp->lParam;
			}
			break;
	}

	return pContainer->DefaultSettingsProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK Explorerplusplus::DefaultSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				if(m_config->defaultFolderSettings.showHidden)
					CheckDlgButton(hDlg,IDC_SHOWHIDDENGLOBAL,BST_CHECKED);

				if(m_config->defaultFolderSettings.showInGroups)
					CheckDlgButton(hDlg,IDC_SHOWINGROUPSGLOBAL,BST_CHECKED);

				if(m_config->defaultFolderSettings.autoArrange)
					CheckDlgButton(hDlg,IDC_AUTOARRANGEGLOBAL,BST_CHECKED);

				if(m_config->defaultFolderSettings.sortAscending)
					CheckDlgButton(hDlg,IDC_SORTASCENDINGGLOBAL,BST_CHECKED);

				HWND hComboBox = GetDlgItem(hDlg,IDC_OPTIONS_DEFAULT_VIEW);
				int SelectedIndex = -1;

				for(auto viewMode : m_viewModes)
				{
					int StringID = GetViewModeMenuStringId(viewMode);

					TCHAR szTemp[64];
					LoadString(m_hLanguageModule,StringID,szTemp,SIZEOF_ARRAY(szTemp));

					int Index = static_cast<int>(SendMessage(hComboBox,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(szTemp)));

					if(Index != CB_ERR)
					{
						SendMessage(hComboBox,CB_SETITEMDATA,Index,viewMode);
					}

					if(viewMode == m_config->defaultFolderSettings.viewMode)
					{
						SelectedIndex = Index;
					}
				}

				SendMessage(hComboBox,CB_SETCURSEL,SelectedIndex,0);
			}
			break;

		case WM_COMMAND:
			if(HIWORD(wParam) != 0)
			{
				switch(HIWORD(wParam))
				{
				case CBN_SELCHANGE:
					PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;
				}
			}
			else
			{
				switch(LOWORD(wParam))
				{
				case IDC_SHOWHIDDENGLOBAL:
				case IDC_AUTOARRANGEGLOBAL:
				case IDC_SORTASCENDINGGLOBAL:
				case IDC_SHOWINGROUPSGLOBAL:
					PropSheet_Changed(g_hOptionsPropertyDialog,hDlg);
					break;

				case IDC_BUTTON_DEFAULTCOLUMNS:
					{
						CSetDefaultColumnsDialog SetDefaultColumnsDialog(m_hLanguageModule,IDD_SETDEFAULTCOLUMNS,hDlg,this,m_config->globalFolderSettings.folderColumns);
						SetDefaultColumnsDialog.ShowModalDialog();
					}
					break;
				}
			}
			break;

		case WM_NOTIFY:
			{
				NMHDR	*nmhdr = NULL;
				nmhdr = (NMHDR *)lParam;

				switch(nmhdr->code)
				{
				case PSN_APPLY:
					{
						m_config->defaultFolderSettings.showHidden = (IsDlgButtonChecked(hDlg,IDC_SHOWHIDDENGLOBAL)
							== BST_CHECKED);

						m_config->defaultFolderSettings.showInGroups = (IsDlgButtonChecked(hDlg,IDC_SHOWINGROUPSGLOBAL)
							== BST_CHECKED);

						m_config->defaultFolderSettings.autoArrange = (IsDlgButtonChecked(hDlg,IDC_AUTOARRANGEGLOBAL)
							== BST_CHECKED);

						m_config->defaultFolderSettings.sortAscending = (IsDlgButtonChecked(hDlg,IDC_SORTASCENDINGGLOBAL)
							== BST_CHECKED);

						HWND hComboBox = GetDlgItem(hDlg,IDC_OPTIONS_DEFAULT_VIEW);
						int SelectedIndex = static_cast<int>(SendMessage(hComboBox,CB_GETCURSEL,0,0));
						m_config->defaultFolderSettings.viewMode = ViewMode::_from_integral(static_cast<int>(SendMessage(hComboBox,CB_GETITEMDATA,SelectedIndex,0)));

						SaveAllSettings();
					}
					break;
				}
			}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void Explorerplusplus::OnDefaultSettingsNewTabDir(HWND hDlg)
{
	BROWSEINFO bi;
	PIDLIST_ABSOLUTE pidl = NULL;
	HWND hEdit;
	TCHAR szDisplayName[MAX_PATH];
	TCHAR szHelper[256];
	TCHAR szNewTabDir[MAX_PATH];
	TCHAR szVirtualParsingPath[MAX_PATH];
	HRESULT hr;

	/* Load the dialog helper message. */
	LoadString(m_hLanguageModule,IDS_DEFAULTSETTINGS_NEWTAB,
		szHelper,SIZEOF_ARRAY(szHelper));

	GetDlgItemText(hDlg,IDC_DEFAULT_NEWTABDIR_EDIT,szNewTabDir,
		SIZEOF_ARRAY(szNewTabDir));

	hr = DecodeFriendlyPath(szNewTabDir,szVirtualParsingPath,SIZEOF_ARRAY(szVirtualParsingPath));

	if(SUCCEEDED(hr))
		StringCchCopy(g_szNewTabDirectory,SIZEOF_ARRAY(g_szNewTabDirectory),
		szVirtualParsingPath);
	else
		StringCchCopy(g_szNewTabDirectory,SIZEOF_ARRAY(g_szNewTabDirectory),
		szNewTabDir);

	CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);

	bi.hwndOwner		= hDlg;
	bi.pidlRoot			= NULL;
	bi.pszDisplayName	= szDisplayName;
	bi.lpszTitle		= szHelper;
	bi.ulFlags			= BIF_NEWDIALOGSTYLE;
	bi.lpfn				= NewTabDirectoryBrowseCallbackProc;

	pidl = SHBrowseForFolder(&bi);

	CoUninitialize();

	if(pidl != NULL)
	{
		hEdit = GetDlgItem(hDlg,IDC_DEFAULT_NEWTABDIR_EDIT);

		DefaultSettingsSetNewTabDir(hEdit,pidl);

		CoTaskMemFree(pidl);
	}
}

int CALLBACK NewTabDirectoryBrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
	UNREFERENCED_PARAMETER(lParam);
	UNREFERENCED_PARAMETER(lpData);

	switch(uMsg)
	{
	case BFFM_INITIALIZED:
		SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)g_szNewTabDirectory);
		break;
	}

	return 0;
}

void Explorerplusplus::DefaultSettingsSetNewTabDir(HWND hEdit, const TCHAR *szPath)
{
	LPITEMIDLIST	pidl = NULL;
	HRESULT			hr;

	hr = GetIdlFromParsingName(szPath,&pidl);

	if(SUCCEEDED(hr))
	{
		DefaultSettingsSetNewTabDir(hEdit,pidl);

		CoTaskMemFree(pidl);
	}
}

void Explorerplusplus::DefaultSettingsSetNewTabDir(HWND hEdit,LPITEMIDLIST pidl)
{
	SFGAOF			Attributes;
	DWORD			uNameFlags;
	TCHAR			szNewTabDir[MAX_PATH];

	Attributes = SFGAO_FILESYSTEM;

	/* Check if the specified folder is real or virtual. */
	GetItemAttributes(pidl,&Attributes);

	if(Attributes & SFGAO_FILESYSTEM)
		uNameFlags = SHGDN_FORPARSING;
	else
		uNameFlags = SHGDN_INFOLDER;

	GetDisplayName(pidl,szNewTabDir,SIZEOF_ARRAY(szNewTabDir),uNameFlags);

	SendMessage(hEdit,WM_SETTEXT,0,(LPARAM)szNewTabDir);
}

void Explorerplusplus::AddLanguages(HWND hDlg)
{
	HWND			hLanguageComboBox;
	WIN32_FIND_DATA	wfd;
	HANDLE			hFindFile;
	TCHAR			szImageDirectory[MAX_PATH];
	TCHAR			szNamePattern[MAX_PATH];
	WORD			wLanguage;
	int				iIndex = 1;
	int				iSel = 0;

	hLanguageComboBox = GetDlgItem(hDlg,IDC_OPTIONS_LANGUAGE);

	/* English will always be added to the combox, and will
	always be the first item. */
	SendMessage(hLanguageComboBox,CB_ADDSTRING,0,(LPARAM)_T("English"));
	SendMessage(hLanguageComboBox,CB_SETITEMDATA,0,9);

	GetProcessImageName(GetCurrentProcessId(),szImageDirectory,SIZEOF_ARRAY(szImageDirectory));
	PathRemoveFileSpec(szImageDirectory);
	StringCchCopy(szNamePattern,SIZEOF_ARRAY(szNamePattern),szImageDirectory);
	PathAppend(szNamePattern, NExplorerplusplus::LANGUAGE_DLL_FILENAME_PATTERN);

	hFindFile = FindFirstFile(szNamePattern,&wfd);

	/* Enumerate all the possible language DLL's. */
	if(hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			BOOL bRet = AddLanguageToComboBox(hLanguageComboBox,
				szImageDirectory,wfd.cFileName,&wLanguage);

			if(bRet)
			{
				if(wLanguage == m_Language)
				{
					iSel = iIndex;
				}

				iIndex++;
			}
		} while(FindNextFile(hFindFile, &wfd));

		FindClose(hFindFile);
	}

	/* Now, select the current language. */
	SendMessage(hLanguageComboBox,CB_SETCURSEL,iSel,0);
}

BOOL Explorerplusplus::AddLanguageToComboBox(HWND hComboBox,
	const TCHAR *szImageDirectory, const TCHAR *szFileName, WORD *pdwLanguage)
{
	TCHAR szFullFileName[MAX_PATH];
	StringCchCopy(szFullFileName, SIZEOF_ARRAY(szFullFileName), szImageDirectory);
	PathAppend(szFullFileName, szFileName);

	BOOL bSuccess = FALSE;
	WORD wLanguage;
	BOOL bRet = GetFileLanguage(szFullFileName, &wLanguage);

	if(bRet)
	{
		TCHAR szLanguageName[32];

		int iRet = GetLocaleInfo(wLanguage, LOCALE_SNATIVELANGNAME,
			szLanguageName, SIZEOF_ARRAY(szLanguageName));

		if(iRet != 0)
		{
			int iIndex = (int) SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) szLanguageName);

			if(iIndex != CB_ERR)
			{
				/* Associate the language identifier with the item. */
				SendMessage(hComboBox, CB_SETITEMDATA, iIndex, wLanguage);

				*pdwLanguage = wLanguage;
				bSuccess = TRUE;
			}
		}
	}

	return bSuccess;
}

int Explorerplusplus::GetLanguageIDFromIndex(HWND hDlg,int iIndex)
{
	HWND	hComboBox;
	int		iLanguage;

	hComboBox = GetDlgItem(hDlg,IDC_OPTIONS_LANGUAGE);

	iLanguage = (int)SendMessage(hComboBox,CB_GETITEMDATA,iIndex,0);

	return iLanguage;
}

void Explorerplusplus::SetInfoTipWindowStates(HWND hDlg)
{
	HWND	hCheckSystemInfoTips;
	HWND	hCheckCustomInfoTips;
	BOOL	bEnable;

	hCheckSystemInfoTips = GetDlgItem(hDlg,IDC_OPTIONS_RADIO_SYSTEMINFOTIPS);
	hCheckCustomInfoTips = GetDlgItem(hDlg,IDC_OPTIONS_RADIO_CUSTOMINFOTIPS);

	bEnable = (IsDlgButtonChecked(hDlg,IDC_OPTIONS_CHECK_SHOWINFOTIPS)
		== BST_CHECKED);

	EnableWindow(hCheckSystemInfoTips,bEnable);
	EnableWindow(hCheckCustomInfoTips,bEnable);
}

void Explorerplusplus::SetFolderSizeWindowState(HWND hDlg)
{
	HWND hFolderSizesNeworkRemovable;
	BOOL bEnable;

	hFolderSizesNeworkRemovable = GetDlgItem(hDlg,IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE);

	bEnable = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_FOLDERSIZES)
		== BST_CHECKED);

	EnableWindow(hFolderSizesNeworkRemovable,bEnable);
}