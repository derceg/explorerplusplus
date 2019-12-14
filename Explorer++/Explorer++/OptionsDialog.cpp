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
#include "OptionsDialog.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ModelessDialogs.h"
#include "ResourceHelper.h"
#include "SetDefaultColumnsDialog.h"
#include "ShellBrowser/ViewModes.h"
#include "ViewModeHelper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include <boost/range/adaptor/map.hpp>

int CALLBACK NewTabDirectoryBrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData);
UINT GetIconThemeStringResourceId(IconTheme iconTheme);

const OptionsDialog::OptionsDialogSheetInfo OptionsDialog::OPTIONS_DIALOG_SHEETS[] = {
	{IDD_OPTIONS_GENERAL, GeneralSettingsProcStub},
	{IDD_OPTIONS_FILESFOLDERS, FilesFoldersProcStub},
	{IDD_OPTIONS_WINDOW, WindowProcStub},
	{IDD_OPTIONS_TABS, TabSettingsProcStub},
	{IDD_OPTIONS_DEFAULT, DefaultSettingsProcStub}
};

struct FileSize_t
{
	SizeDisplayFormat_t sdf;
	UINT StringID;
};

static const FileSize_t FILE_SIZES[] = {
	{SIZE_FORMAT_BYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_BYTES},
	{SIZE_FORMAT_KBYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_KB},
	{SIZE_FORMAT_MBYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_MB},
	{SIZE_FORMAT_GBYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_GB},
	{SIZE_FORMAT_TBYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_TB},
	{SIZE_FORMAT_PBYTES, IDS_OPTIONS_DIALOG_FILE_SIZE_PB}
};

TCHAR g_szNewTabDirectory[MAX_PATH];

OptionsDialog *OptionsDialog::Create(std::shared_ptr<Config> config, HINSTANCE instance,
	IExplorerplusplus *expp, TabContainer *tabContainer)
{
	return new OptionsDialog(config, instance, expp, tabContainer);
}

OptionsDialog::OptionsDialog(std::shared_ptr<Config> config, HINSTANCE instance,
	IExplorerplusplus *expp, TabContainer *tabContainer) :
	m_config(config),
	m_instance(instance),
	m_expp(expp),
	m_tabContainer(tabContainer)
{

}

HWND OptionsDialog::Show(HWND parentWindow)
{
	std::vector<HPROPSHEETPAGE> sheetHandles;

	for (const auto &optionDialogSheet : OPTIONS_DIALOG_SHEETS)
	{
		PROPSHEETPAGE sheet = GeneratePropertySheetDefinition(optionDialogSheet);
		sheetHandles.push_back(CreatePropertySheetPage(&sheet));
	}

	std::wstring title = ResourceHelper::LoadString(m_instance, IDS_OPTIONSDIALOG_TITLE);

	UINT dpi = m_dpiCompat.GetDpiForWindow(parentWindow);
	int iconWidth = m_dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = m_dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	m_optionsDialogIcon = m_expp->GetIconResourceLoader()->LoadIconFromPNGForDpi(Icon::Options, iconWidth, iconHeight, dpi);

	PROPSHEETHEADER psh;
	psh.dwSize		= sizeof(PROPSHEETHEADER);
	psh.dwFlags		= PSH_DEFAULT|PSH_USECALLBACK|PSH_NOCONTEXTHELP|PSH_USEHICON|PSH_MODELESS;
	psh.hwndParent	= parentWindow;
	psh.pszCaption	= title.c_str();
	psh.nPages		= static_cast<UINT>(sheetHandles.size());
	psh.nStartPage	= 0;
	psh.hIcon		= m_optionsDialogIcon.get();
	psh.ppsp		= nullptr;
	psh.phpage		= sheetHandles.data();
	psh.pfnCallback	= nullptr;
	HWND propertySheet = reinterpret_cast<HWND>(PropertySheet(&psh));

	m_windowSubclasses.push_back(WindowSubclassWrapper(propertySheet, PropSheetProcStub,
		PROP_SHEET_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	CenterWindow(parentWindow, propertySheet);

	return propertySheet;
}

PROPSHEETPAGE OptionsDialog::GeneratePropertySheetDefinition(const OptionsDialogSheetInfo &sheetInfo)
{
	PROPSHEETPAGE sheet;
	sheet.dwSize = sizeof(PROPSHEETPAGE);
	sheet.dwFlags = PSP_DEFAULT;
	sheet.hInstance = m_instance;
	sheet.pszTemplate = MAKEINTRESOURCE(sheetInfo.resourceId);
	sheet.lParam = reinterpret_cast<LPARAM>(this);
	sheet.pfnDlgProc = sheetInfo.dlgProc;
	return sheet;
}

LRESULT CALLBACK OptionsDialog::PropSheetProcStub(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	OptionsDialog *optionsDialog = reinterpret_cast<OptionsDialog *>(dwRefData);
	return optionsDialog->PropSheetProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK OptionsDialog::PropSheetProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCDESTROY:
		delete this;
		return 0;
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK OptionsDialog::GeneralSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				PROPSHEETPAGE *ppsp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
				optionsDialog = reinterpret_cast<OptionsDialog *>(ppsp->lParam);
			}
			break;
	}

	return optionsDialog->GeneralSettingsProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK OptionsDialog::GeneralSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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

				if (m_expp->GetSavePreferencesToXmlFile())
				{
					CheckDlgButton(hDlg, IDC_OPTION_XML, BST_CHECKED);
				}

				UINT dpi = m_dpiCompat.GetDpiForWindow(hDlg);
				m_newTabDirectoryIcon = m_expp->GetIconResourceLoader()->LoadIconFromPNGForDpi(Icon::Folder, 16, 16, dpi);

				hButton = GetDlgItem(hDlg,IDC_DEFAULT_NEWTABDIR_BUTTON);
				SendMessage(hButton,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_newTabDirectoryIcon.get());

				hEdit = GetDlgItem(hDlg,IDC_DEFAULT_NEWTABDIR_EDIT);
				DefaultSettingsSetNewTabDir(hEdit,m_config->defaultTabDirectory.c_str());

				AddIconThemes(hDlg);
				AddLanguages(hDlg);
			}
			break;

		case WM_COMMAND:
			if(HIWORD(wParam) != 0)
			{
				switch(HIWORD(wParam))
				{
				case EN_CHANGE:
				case CBN_SELCHANGE:
					PropSheet_Changed(GetParent(hDlg),hDlg);
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
						PropSheet_Changed(GetParent(hDlg),hDlg);
					break;

				case IDC_OPTION_REPLACEEXPLORER_NONE:
				case IDC_OPTION_REPLACEEXPLORER_FILESYSTEM:
				case IDC_OPTION_REPLACEEXPLORER_ALL:
				case IDC_OPTION_XML:
					PropSheet_Changed(GetParent(hDlg),hDlg);
					break;

				case IDC_DEFAULT_NEWTABDIR_BUTTON:
					OnDefaultSettingsNewTabDir(hDlg);
					PropSheet_Changed(GetParent(hDlg),hDlg);
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

							std::wstring menuText = ResourceHelper::LoadString(m_instance, IDS_OPEN_IN_EXPLORERPLUSPLUS);

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
									SHELL_DEFAULT_INTERNAL_COMMAND_NAME, menuText.c_str());
								break;

							case NDefaultFileManager::REPLACEEXPLORER_ALL:
								NDefaultFileManager::RemoveAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
								NDefaultFileManager::RemoveAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
								bSuccess = NDefaultFileManager::SetAsDefaultFileManagerAll(
									SHELL_DEFAULT_INTERNAL_COMMAND_NAME, menuText.c_str());
								break;
							}

							if(bSuccess)
							{
								m_config->replaceExplorerMode = ReplaceExplorerMode;
							}
							else
							{
								std::wstring errorMessage = ResourceHelper::LoadString(m_instance, IDS_ERR_FILEMANAGERSETTING);
								MessageBox(hDlg, errorMessage.c_str(), NExplorerplusplus::APP_NAME, MB_ICONWARNING);

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

						BOOL savePreferencesToXmlFile = (IsDlgButtonChecked(hDlg, IDC_OPTION_XML) == BST_CHECKED);
						m_expp->SetSavePreferencesToXmlFile(savePreferencesToXmlFile);

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

						iSel = static_cast<int>(SendDlgItemMessage(hDlg, IDC_OPTIONS_ICON_THEME, CB_GETCURSEL, 0, 0));
						int iconThemeItemData = static_cast<int>(SendDlgItemMessage(hDlg, IDC_OPTIONS_ICON_THEME, CB_GETITEMDATA, iSel, 0));
						m_config->iconTheme = IconTheme::_from_integral(iconThemeItemData);

						iSel = static_cast<int>(SendDlgItemMessage(hDlg,IDC_OPTIONS_LANGUAGE,CB_GETCURSEL,0,0));

						int language = GetLanguageIDFromIndex(hDlg, iSel);
						m_config->language = language;

						m_expp->SaveAllSettings();
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

INT_PTR CALLBACK OptionsDialog::FilesFoldersProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				PROPSHEETPAGE *ppsp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
				optionsDialog = reinterpret_cast<OptionsDialog *>(ppsp->lParam);
			}
			break;
	}

	return optionsDialog->FilesFoldersProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK OptionsDialog::FilesFoldersProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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
					std::wstring fileSizeText = ResourceHelper::LoadString(m_instance,FILE_SIZES[i].StringID);
					SendMessage(hCBSize,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(fileSizeText.c_str()));
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
					PropSheet_Changed(GetParent(hDlg),hDlg);
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
					PropSheet_Changed(GetParent(hDlg),hDlg);
					break;

				case IDC_SETTINGS_CHECK_FORCESIZE:
					EnableWindow(GetDlgItem(hDlg,IDC_COMBO_FILESIZES),IsDlgButtonChecked(hDlg,LOWORD(wParam)) == BST_CHECKED);
					PropSheet_Changed(GetParent(hDlg),hDlg);
					break;

				case IDC_OPTIONS_RADIO_SYSTEMINFOTIPS:
				case IDC_OPTIONS_RADIO_CUSTOMINFOTIPS:
					if(IsDlgButtonChecked(hDlg,LOWORD(wParam)) == BST_CHECKED)
						PropSheet_Changed(GetParent(hDlg),hDlg);
					break;

				case IDC_OPTIONS_CHECK_SHOWINFOTIPS:
					SetInfoTipWindowStates(hDlg);
					PropSheet_Changed(GetParent(hDlg),hDlg);
					break;

				case IDC_SETTINGS_CHECK_FOLDERSIZES:
					SetFolderSizeWindowState(hDlg);
					PropSheet_Changed(GetParent(hDlg),hDlg);
					break;

				case IDC_SETTINGS_CHECK_SINGLECLICK:
					EnableWindow(GetDlgItem(hDlg,IDC_OPTIONS_HOVER_TIME),IsDlgButtonChecked(hDlg,LOWORD(wParam)) == BST_CHECKED);
					EnableWindow(GetDlgItem(hDlg,IDC_LABEL_HOVER_TIME),IsDlgButtonChecked(hDlg,LOWORD(wParam)) == BST_CHECKED);
					PropSheet_Changed(GetParent(hDlg),hDlg);
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

						m_config->globalFolderSettings.disableFolderSizesNetworkRemovable =
							(IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE)
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
							tab->GetShellBrowser()->GetNavigationController()->Refresh();

							NListView::ListView_ActivateOneClickSelect(tab->GetShellBrowser()->GetListView(),
								m_config->globalFolderSettings.oneClickActivate,
								m_config->globalFolderSettings.oneClickActivateHoverTime);
						}

						m_expp->SaveAllSettings();
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

INT_PTR CALLBACK OptionsDialog::WindowProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				PROPSHEETPAGE *ppsp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
				optionsDialog = reinterpret_cast<OptionsDialog *>(ppsp->lParam);
			}
			break;
	}

	return optionsDialog->WindowProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK OptionsDialog::WindowProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			if(m_config->allowMultipleInstances)
				CheckDlgButton(hDlg,IDC_OPTION_MULTIPLEINSTANCES,BST_CHECKED);
			if(m_config->useLargeToolbarIcons.get())
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
			PropSheet_Changed(GetParent(hDlg),hDlg);
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
							DWORD dwExtendedStyle = ListView_GetExtendedListViewStyle(tab->GetShellBrowser()->GetListView());

							if(bCheckBoxSelection)
							{
								dwExtendedStyle |= LVS_EX_CHECKBOXES;
							}
							else
							{
								dwExtendedStyle &= ~LVS_EX_CHECKBOXES;
							}

							ListView_SetExtendedListViewStyle(tab->GetShellBrowser()->GetListView(), dwExtendedStyle);
						}

						m_config->checkBoxSelection = (IsDlgButtonChecked(hDlg,IDC_OPTION_CHECKBOXSELECTION)
							== BST_CHECKED);
					}

					m_config->useFullRowSelect = (IsDlgButtonChecked(hDlg,IDC_OPTION_FULLROWSELECT)
						== BST_CHECKED);

					m_config->useLargeToolbarIcons.set(IsDlgButtonChecked(hDlg,IDC_OPTION_LARGETOOLBARICONS)
						== BST_CHECKED);

					for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
					{
						/* TODO: The tab should monitor for settings
						changes itself. */
						tab->GetShellBrowser()->OnGridlinesSettingChanged();

						NListView::ListView_AddRemoveExtendedStyle(tab->GetShellBrowser()->GetListView(),
							LVS_EX_FULLROWSELECT,m_config->useFullRowSelect);
					}

					m_expp->SaveAllSettings();
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

INT_PTR CALLBACK OptionsDialog::TabSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				PROPSHEETPAGE *ppsp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
				optionsDialog = reinterpret_cast<OptionsDialog *>(ppsp->lParam);
			}
			break;
	}

	return optionsDialog->TabSettingsProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK OptionsDialog::TabSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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
				PropSheet_Changed(GetParent(hDlg),hDlg);
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

						m_expp->SaveAllSettings();
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

INT_PTR CALLBACK OptionsDialog::DefaultSettingsProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			{
				PROPSHEETPAGE *ppsp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
				optionsDialog = reinterpret_cast<OptionsDialog *>(ppsp->lParam);
			}
			break;
	}

	return optionsDialog->DefaultSettingsProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK OptionsDialog::DefaultSettingsProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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

				for(auto viewMode : VIEW_MODES)
				{
					int StringID = GetViewModeMenuStringId(viewMode);

					std::wstring viewModeText = ResourceHelper::LoadString(m_instance, StringID);

					int Index = static_cast<int>(SendMessage(hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(viewModeText.c_str())));

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
					PropSheet_Changed(GetParent(hDlg),hDlg);
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
					PropSheet_Changed(GetParent(hDlg),hDlg);
					break;

				case IDC_BUTTON_DEFAULTCOLUMNS:
					{
						CSetDefaultColumnsDialog SetDefaultColumnsDialog(m_instance,
							hDlg, m_config->globalFolderSettings.folderColumns);
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
						m_config->defaultFolderSettings.viewMode = ViewMode::_from_integral(
							static_cast<int>(SendMessage(hComboBox, CB_GETITEMDATA, SelectedIndex, 0)));

						m_expp->SaveAllSettings();
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

void OptionsDialog::OnDefaultSettingsNewTabDir(HWND hDlg)
{
	BROWSEINFO bi;
	HWND hEdit;
	TCHAR szDisplayName[MAX_PATH];
	TCHAR szNewTabDir[MAX_PATH];
	TCHAR szVirtualParsingPath[MAX_PATH];
	HRESULT hr;

	/* Load the dialog helper message. */
	std::wstring helperText = ResourceHelper::LoadString(m_instance,IDS_DEFAULTSETTINGS_NEWTAB);

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
	bi.lpszTitle		= helperText.c_str();
	bi.ulFlags			= BIF_NEWDIALOGSTYLE;
	bi.lpfn				= NewTabDirectoryBrowseCallbackProc;

	unique_pidl_absolute pidl(SHBrowseForFolder(&bi));

	CoUninitialize();

	if(pidl != NULL)
	{
		hEdit = GetDlgItem(hDlg,IDC_DEFAULT_NEWTABDIR_EDIT);

		DefaultSettingsSetNewTabDir(hEdit,pidl.get());
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

void OptionsDialog::DefaultSettingsSetNewTabDir(HWND hEdit, const TCHAR *szPath)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(szPath, nullptr, wil::out_param(pidl), 0, nullptr);

	if(SUCCEEDED(hr))
	{
		DefaultSettingsSetNewTabDir(hEdit,pidl.get());
	}
}

void OptionsDialog::DefaultSettingsSetNewTabDir(HWND hEdit, PCIDLIST_ABSOLUTE pidl)
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

void OptionsDialog::AddIconThemes(HWND dlg)
{
	HWND iconThemeControl = GetDlgItem(dlg, IDC_OPTIONS_ICON_THEME);
	int currentThemeIndex = 0;

	for (auto theme : IconTheme::_values())
	{
		UINT stringResourceId = GetIconThemeStringResourceId(theme);
		std::wstring iconThemeName = ResourceHelper::LoadString(m_instance, stringResourceId);

		int index = static_cast<int>(SendMessage(iconThemeControl, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(iconThemeName.c_str())));

		if (index == CB_ERR)
		{
			continue;
		}

		SendMessage(iconThemeControl, CB_SETITEMDATA, index, theme);

		if (theme == m_config->iconTheme)
		{
			currentThemeIndex = index;
		}
	}

	SendMessage(iconThemeControl, CB_SETCURSEL, currentThemeIndex, 0);
}

UINT GetIconThemeStringResourceId(IconTheme iconTheme)
{
	switch (iconTheme)
	{
	case IconTheme::Color:
		return IDS_ICON_THEME_COLOR;
		break;

	case IconTheme::Windows10:
		return IDS_ICON_THEME_WINDOWS_10;
		break;

	default:
		throw std::runtime_error("IconTheme value not found");
		break;
	}
}

void OptionsDialog::AddLanguages(HWND hDlg)
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
				if(wLanguage == m_config->language)
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

BOOL OptionsDialog::AddLanguageToComboBox(HWND hComboBox, const TCHAR *szImageDirectory,
	const TCHAR *szFileName, WORD *pdwLanguage)
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

int OptionsDialog::GetLanguageIDFromIndex(HWND hDlg,int iIndex)
{
	HWND	hComboBox;
	int		iLanguage;

	hComboBox = GetDlgItem(hDlg,IDC_OPTIONS_LANGUAGE);

	iLanguage = (int)SendMessage(hComboBox,CB_GETITEMDATA,iIndex,0);

	return iLanguage;
}

void OptionsDialog::SetInfoTipWindowStates(HWND hDlg)
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

void OptionsDialog::SetFolderSizeWindowState(HWND hDlg)
{
	HWND hFolderSizesNeworkRemovable;
	BOOL bEnable;

	hFolderSizesNeworkRemovable = GetDlgItem(hDlg,IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE);

	bEnable = (IsDlgButtonChecked(hDlg,IDC_SETTINGS_CHECK_FOLDERSIZES)
		== BST_CHECKED);

	EnableWindow(hFolderSizesNeworkRemovable,bEnable);
}