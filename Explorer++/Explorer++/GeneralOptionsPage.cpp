// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GeneralOptionsPage.h"
#include "App.h"
#include "Config.h"
#include "CoreInterface.h"
#include "Explorer++_internal.h"
#include "LanguageHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ResizableDialogHelper.h"
#include "../Helper/WindowHelper.h"
#include <fmt/format.h>
#include <fmt/xchar.h>

using namespace DefaultFileManager;

const std::unordered_map<ReplaceExplorerMode, int> REPLACE_EXPLORER_ENUM_CONTROL_ID_MAPPINGS = {
	{ ReplaceExplorerMode::None, IDC_OPTION_REPLACEEXPLORER_NONE },
	{ ReplaceExplorerMode::FileSystem, IDC_OPTION_REPLACEEXPLORER_FILESYSTEM },
	{ ReplaceExplorerMode::All, IDC_OPTION_REPLACEEXPLORER_ALL }
};

GeneralOptionsPage::GeneralOptionsPage(HWND parent, HINSTANCE resourceInstance, App *app,
	Config *config, CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback,
	HWND tooltipWindow) :
	OptionsPage(IDD_OPTIONS_GENERAL, IDS_OPTIONS_GENERAL_TITLE, parent, resourceInstance, config,
		coreInterface, settingChangedCallback, tooltipWindow),
	m_app(app)
{
}

std::unique_ptr<ResizableDialogHelper> GeneralOptionsPage::InitializeResizeDialogHelper()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_GROUP_STARTUP), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STARTUP_PREVIOUSTABS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STARTUP_DEFAULTFOLDER), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_GROUP_DEFAULT_FILE_MANAGER), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_REPLACEEXPLORER_NONE),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_REPLACEEXPLORER_FILESYSTEM),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_REPLACEEXPLORER_ALL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_GROUP_GENERAL_SETTINGS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTION_XML), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STATIC_NEW_TAB_FOLDER), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_EDIT), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_BUTTON),
		MovingType::Horizontal, SizingType::None);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_OPTIONS_LANGUAGE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STATIC_RESTART_FOOTNOTE),
		MovingType::Horizontal, SizingType::None);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STATIC_RESTART_NOTICE), MovingType::None,
		SizingType::Horizontal);
	return std::make_unique<ResizableDialogHelper>(GetDialog(), controls);
}

void GeneralOptionsPage::InitializeControls()
{
	HWND hButton;
	HWND hEdit;
	int nIDButton;

	switch (m_config->startupMode)
	{
	case StartupMode::PreviousTabs:
		nIDButton = IDC_STARTUP_PREVIOUSTABS;
		break;

	case StartupMode::DefaultFolder:
		nIDButton = IDC_STARTUP_DEFAULTFOLDER;
		break;

	default:
		nIDButton = IDC_STARTUP_PREVIOUSTABS;
		m_config->startupMode = StartupMode::PreviousTabs;
		break;
	}
	CheckDlgButton(GetDialog(), nIDButton, BST_CHECKED);

	CheckRadioButton(GetDialog(), IDC_OPTION_REPLACEEXPLORER_NONE, IDC_OPTION_REPLACEEXPLORER_ALL,
		REPLACE_EXPLORER_ENUM_CONTROL_ID_MAPPINGS.at(m_config->replaceExplorerMode));

	if (m_app->GetSavePreferencesToXmlFile())
	{
		CheckDlgButton(GetDialog(), IDC_OPTION_XML, BST_CHECKED);
	}

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(GetDialog());
	m_newTabDirectoryIcon =
		m_app->GetIconResourceLoader()->LoadIconFromPNGForDpi(Icon::Folder, 16, 16, dpi);

	hButton = GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_BUTTON);
	SendMessage(hButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM) m_newTabDirectoryIcon.get());

	hEdit = GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_EDIT);
	SetNewTabDirectory(hEdit, m_config->defaultTabDirectory.c_str());

	AddLanguages();
}

void GeneralOptionsPage::SetNewTabDirectory(HWND hEdit, const TCHAR *szPath)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(szPath, nullptr, wil::out_param(pidl), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		SetNewTabDirectory(hEdit, pidl.get());
	}
}

void GeneralOptionsPage::SetNewTabDirectory(HWND hEdit, PCIDLIST_ABSOLUTE pidl)
{
	/* Check if the specified folder is real or virtual. */
	SFGAOF attributes = SFGAO_FILESYSTEM;
	GetItemAttributes(pidl, &attributes);

	DWORD nameFlags;

	if (attributes & SFGAO_FILESYSTEM)
	{
		nameFlags = SHGDN_FORPARSING;
	}
	else
	{
		nameFlags = SHGDN_INFOLDER;
	}

	std::wstring newTabDir;
	GetDisplayName(pidl, nameFlags, newTabDir);

	SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) newTabDir.c_str());
}

void GeneralOptionsPage::AddLanguages()
{
	HWND hLanguageComboBox;
	WIN32_FIND_DATA wfd;
	HANDLE hFindFile;
	TCHAR szImageDirectory[MAX_PATH];
	TCHAR szNamePattern[MAX_PATH];
	WORD wLanguage;
	int iIndex = 1;
	int iSel = 0;

	hLanguageComboBox = GetDlgItem(GetDialog(), IDC_OPTIONS_LANGUAGE);

	/* English will always be added to the combox, and will
	always be the first item. */
	SendMessage(hLanguageComboBox, CB_ADDSTRING, 0, (LPARAM) _T("English"));

	GetProcessImageName(GetCurrentProcessId(), szImageDirectory, SIZEOF_ARRAY(szImageDirectory));
	PathRemoveFileSpec(szImageDirectory);
	StringCchCopy(szNamePattern, SIZEOF_ARRAY(szNamePattern), szImageDirectory);
	PathAppend(szNamePattern, L"Explorer++*.dll");

	hFindFile = FindFirstFile(szNamePattern, &wfd);

	/* Enumerate all the possible language DLL's. */
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			BOOL bRet = AddLanguageToComboBox(hLanguageComboBox, szImageDirectory, wfd.cFileName,
				&wLanguage);

			if (bRet)
			{
				if (wLanguage == m_config->language)
				{
					iSel = iIndex;
				}

				iIndex++;
			}
		} while (FindNextFile(hFindFile, &wfd));

		FindClose(hFindFile);
	}

	/* Now, select the current language. */
	SendMessage(hLanguageComboBox, CB_SETCURSEL, iSel, 0);
}

BOOL GeneralOptionsPage::AddLanguageToComboBox(HWND hComboBox, const TCHAR *szImageDirectory,
	const TCHAR *szFileName, WORD *pdwLanguage)
{
	TCHAR szFullFileName[MAX_PATH];
	StringCchCopy(szFullFileName, SIZEOF_ARRAY(szFullFileName), szImageDirectory);
	PathAppend(szFullFileName, szFileName);

	BOOL bSuccess = FALSE;
	WORD wLanguage;
	BOOL bRet = GetFileLanguage(szFullFileName, &wLanguage);

	if (bRet)
	{
		TCHAR szLanguageName[32];

		int iRet = GetLocaleInfo(wLanguage, LOCALE_SNATIVELANGNAME, szLanguageName,
			SIZEOF_ARRAY(szLanguageName));

		if (iRet != 0)
		{
			int iIndex = (int) SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) szLanguageName);

			if (iIndex != CB_ERR)
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

void GeneralOptionsPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) != 0)
	{
		switch (HIWORD(wParam))
		{
		case EN_CHANGE:
		case CBN_SELCHANGE:
			m_settingChangedCallback();
			break;
		}
	}
	else
	{
		switch (LOWORD(wParam))
		{
		case IDC_STARTUP_PREVIOUSTABS:
		case IDC_STARTUP_DEFAULTFOLDER:
			if (IsDlgButtonChecked(GetDialog(), LOWORD(wParam)) == BST_CHECKED)
			{
				m_settingChangedCallback();
			}
			break;

		case IDC_OPTION_REPLACEEXPLORER_NONE:
		case IDC_OPTION_REPLACEEXPLORER_FILESYSTEM:
		case IDC_OPTION_REPLACEEXPLORER_ALL:
		case IDC_OPTION_XML:
			m_settingChangedCallback();
			break;

		case IDC_DEFAULT_NEWTABDIR_BUTTON:
			OnNewTabDirectoryButtonPressed();
			break;
		}
	}
}

void GeneralOptionsPage::OnNewTabDirectoryButtonPressed()
{
	std::wstring helperText =
		ResourceHelper::LoadString(m_resourceInstance, IDS_DEFAULTSETTINGS_NEWTAB);

	TCHAR szNewTabDir[MAX_PATH];
	GetDlgItemText(GetDialog(), IDC_DEFAULT_NEWTABDIR_EDIT, szNewTabDir, SIZEOF_ARRAY(szNewTabDir));

	std::wstring virtualParsingPath;
	HRESULT hr = DecodeFriendlyPath(szNewTabDir, virtualParsingPath);

	if (SUCCEEDED(hr))
	{
		m_newTabDirectory = virtualParsingPath;
	}
	else
	{
		m_newTabDirectory = szNewTabDir;
	}

	TCHAR szDisplayName[MAX_PATH];
	BROWSEINFO browseInfo = {};
	browseInfo.hwndOwner = GetDialog();
	browseInfo.pidlRoot = nullptr;
	browseInfo.pszDisplayName = szDisplayName;
	browseInfo.lpszTitle = helperText.c_str();
	browseInfo.ulFlags = BIF_NEWDIALOGSTYLE;
	browseInfo.lpfn = BrowseFolderCallbackStub;
	browseInfo.lParam = reinterpret_cast<LPARAM>(this);
	unique_pidl_absolute pidl(SHBrowseForFolder(&browseInfo));

	if (pidl)
	{
		HWND editControl = GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_EDIT);
		SetNewTabDirectory(editControl, pidl.get());
	}
}

int CALLBACK GeneralOptionsPage::BrowseFolderCallbackStub(HWND hwnd, UINT msg, LPARAM lParam,
	LPARAM lpData)
{
	UNREFERENCED_PARAMETER(lParam);

	auto *page = reinterpret_cast<GeneralOptionsPage *>(lpData);
	page->BrowseFolderCallback(hwnd, msg, lParam);

	return 0;
}

void GeneralOptionsPage::BrowseFolderCallback(HWND hwnd, UINT msg, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (msg)
	{
	case BFFM_INITIALIZED:
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE,
			reinterpret_cast<LPARAM>(m_newTabDirectory.c_str()));
		break;
	}
}

void GeneralOptionsPage::SaveSettings()
{
	ReplaceExplorerMode replaceExplorerMode = ReplaceExplorerMode::None;

	if (IsDlgButtonChecked(GetDialog(), IDC_STARTUP_PREVIOUSTABS) == BST_CHECKED)
	{
		m_config->startupMode = StartupMode::PreviousTabs;
	}
	else if (IsDlgButtonChecked(GetDialog(), IDC_STARTUP_DEFAULTFOLDER) == BST_CHECKED)
	{
		m_config->startupMode = StartupMode::DefaultFolder;
	}

	if (IsDlgButtonChecked(GetDialog(), IDC_OPTION_REPLACEEXPLORER_NONE) == BST_CHECKED)
	{
		replaceExplorerMode = ReplaceExplorerMode::None;
	}
	else if (IsDlgButtonChecked(GetDialog(), IDC_OPTION_REPLACEEXPLORER_FILESYSTEM) == BST_CHECKED)
	{
		replaceExplorerMode = ReplaceExplorerMode::FileSystem;
	}
	else if (IsDlgButtonChecked(GetDialog(), IDC_OPTION_REPLACEEXPLORER_ALL) == BST_CHECKED)
	{
		replaceExplorerMode = ReplaceExplorerMode::All;
	}

	if (m_config->replaceExplorerMode != replaceExplorerMode)
	{
		OnReplaceExplorerSettingChanged(replaceExplorerMode);
	}

	bool savePreferencesToXmlFile =
		(IsDlgButtonChecked(GetDialog(), IDC_OPTION_XML) == BST_CHECKED);
	m_app->SetSavePreferencesToXmlFile(savePreferencesToXmlFile);

	HWND hEdit = GetDlgItem(GetDialog(), IDC_DEFAULT_NEWTABDIR_EDIT);
	std::wstring newTabDir = GetWindowString(hEdit);

	/* The folder may be virtual, in which case, it needs
	to be decoded. */
	std::wstring virtualParsingPath;
	HRESULT hr = DecodeFriendlyPath(newTabDir, virtualParsingPath);

	if (SUCCEEDED(hr))
	{
		m_config->defaultTabDirectory = virtualParsingPath;
	}
	else
	{
		m_config->defaultTabDirectory = newTabDir;
	}

	HWND comboBox = GetDlgItem(GetDialog(), IDC_OPTIONS_LANGUAGE);
	auto selectedIndex = static_cast<int>(SendMessage(comboBox, CB_GETCURSEL, 0, 0));

	if (selectedIndex == 0)
	{
		// The first item is always the default language.
		m_config->language = LanguageHelper::DEFAULT_LANGUAGE;
	}
	else
	{
		auto language = static_cast<WORD>(SendMessage(comboBox, CB_GETITEMDATA, selectedIndex, 0));
		m_config->language = language;
	}
}

void GeneralOptionsPage::OnReplaceExplorerSettingChanged(ReplaceExplorerMode updatedReplaceMode)
{
	bool settingChanged = UpdateReplaceExplorerSetting(updatedReplaceMode);

	if (!settingChanged)
	{
		// The default file manager setting was not changed, so reset the state of the file
		// manager radio buttons.
		CheckRadioButton(GetDialog(), IDC_OPTION_REPLACEEXPLORER_NONE,
			IDC_OPTION_REPLACEEXPLORER_ALL,
			REPLACE_EXPLORER_ENUM_CONTROL_ID_MAPPINGS.at(m_config->replaceExplorerMode));
	}
}

bool GeneralOptionsPage::UpdateReplaceExplorerSetting(ReplaceExplorerMode updatedReplaceMode)
{
	if (updatedReplaceMode != +ReplaceExplorerMode::None
		&& m_config->replaceExplorerMode == +ReplaceExplorerMode::None)
	{
		std::wstring warningMessage = ResourceHelper::LoadString(m_resourceInstance,
			IDS_OPTIONS_DIALOG_REPLACE_EXPLORER_WARNING);

		int selectedButton = MessageBox(GetDialog(), warningMessage.c_str(), App::APP_NAME,
			MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);

		if (selectedButton == IDNO)
		{
			return false;
		}
	}

	LSTATUS res = ERROR_SUCCESS;
	std::wstring menuText =
		ResourceHelper::LoadString(m_resourceInstance, IDS_OPEN_IN_EXPLORERPLUSPLUS);

	switch (updatedReplaceMode)
	{
	case ReplaceExplorerMode::None:
	{
		switch (m_config->replaceExplorerMode)
		{
		case ReplaceExplorerMode::FileSystem:
			res = RemoveAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
			break;

		case ReplaceExplorerMode::All:
			res = RemoveAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
			break;
		}
	}
	break;

	case ReplaceExplorerMode::FileSystem:
		RemoveAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
		RemoveAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
		res = SetAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME,
			menuText.c_str());
		break;

	case ReplaceExplorerMode::All:
		RemoveAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
		RemoveAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
		res = SetAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME, menuText.c_str());
		break;
	}

	if (res == ERROR_SUCCESS)
	{
		m_config->replaceExplorerMode = updatedReplaceMode;

		return true;
	}
	else
	{
		auto systemErrorMessage = GetLastErrorMessage(res);
		std::wstring finalSystemErrorMessage;

		if (systemErrorMessage)
		{
			finalSystemErrorMessage = *systemErrorMessage;

			// Any trailing newlines are unnecessary, as they'll be added below when
			// appropriate.
			boost::trim(finalSystemErrorMessage);
		}
		else
		{
			std::wstring errorCodeTemplate =
				ResourceHelper::LoadString(m_resourceInstance, IDS_ERROR_CODE);
			finalSystemErrorMessage =
				fmt::format(fmt::runtime(errorCodeTemplate), fmt::arg(L"error_code", res));
		}

		std::wstring errorMessage =
			ResourceHelper::LoadString(m_resourceInstance, IDS_ERROR_REPLACE_EXPLORER_SETTING)
			+ L"\n\n" + finalSystemErrorMessage;

		MessageBox(GetDialog(), errorMessage.c_str(), App::APP_NAME, MB_ICONWARNING);

		return false;
	}
}
