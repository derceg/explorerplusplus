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
#include "CoreInterface.h"
#include "DarkModeButton.h"
#include "DarkModeGroupBox.h"
#include "DarkModeHelper.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "SetDefaultColumnsDialog.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "TabContainer.h"
#include "ViewModeHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/PropertySheet.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/range/adaptor/map.hpp>
#include <unordered_map>

using namespace DarkModeButton;
using namespace DefaultFileManager;

int CALLBACK NewTabDirectoryBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
UINT GetIconThemeStringResourceId(IconTheme iconTheme);

// clang-format off
const OptionsDialog::OptionsDialogSheetInfo OptionsDialog::OPTIONS_DIALOG_SHEETS[] = {
	{IDD_OPTIONS_GENERAL, GeneralSettingsProcStub},
	{IDD_OPTIONS_FILESFOLDERS, FilesFoldersProcStub},
	{IDD_OPTIONS_WINDOW, WindowProcStub},
	{IDD_OPTIONS_TABS, TabSettingsProcStub},
	{IDD_OPTIONS_DEFAULT, DefaultSettingsProcStub}
};
// clang-format on

const std::unordered_map<ReplaceExplorerMode, int> REPLACE_EXPLORER_ENUM_CONTROL_ID_MAPPINGS = {
	{ ReplaceExplorerMode::None, IDC_OPTION_REPLACEEXPLORER_NONE },
	{ ReplaceExplorerMode::FileSystem, IDC_OPTION_REPLACEEXPLORER_FILESYSTEM },
	{ ReplaceExplorerMode::All, IDC_OPTION_REPLACEEXPLORER_ALL }
};

struct FileSize
{
	SizeDisplayFormat sdf;
	UINT StringID;
};

// clang-format off
static const FileSize FILE_SIZES[] = {
	{SizeDisplayFormat::Bytes, IDS_OPTIONS_DIALOG_FILE_SIZE_BYTES},
	{SizeDisplayFormat::KB, IDS_OPTIONS_DIALOG_FILE_SIZE_KB},
	{SizeDisplayFormat::MB, IDS_OPTIONS_DIALOG_FILE_SIZE_MB},
	{SizeDisplayFormat::GB, IDS_OPTIONS_DIALOG_FILE_SIZE_GB},
	{SizeDisplayFormat::TB, IDS_OPTIONS_DIALOG_FILE_SIZE_TB},
	{SizeDisplayFormat::PB, IDS_OPTIONS_DIALOG_FILE_SIZE_PB}
};
// clang-format on

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

	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(parentWindow);
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	m_optionsDialogIcon = m_expp->GetIconResourceLoader()->LoadIconFromPNGForDpi(
		Icon::Options, iconWidth, iconHeight, dpi);

	PROPSHEETHEADER psh;
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_DEFAULT | PSH_USECALLBACK | PSH_NOCONTEXTHELP | PSH_USEHICON | PSH_MODELESS;
	psh.hwndParent = parentWindow;
	psh.pszCaption = title.c_str();
	psh.nPages = static_cast<UINT>(sheetHandles.size());
	psh.nStartPage = m_lastSelectedSheetIndex;
	psh.hIcon = m_optionsDialogIcon.get();
	psh.ppsp = nullptr;
	psh.phpage = sheetHandles.data();
	psh.pfnCallback = PropertySheetCallback;
	HWND propertySheet = reinterpret_cast<HWND>(PropertySheet(&psh));

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(propertySheet,
		PropSheetProcStub, PROP_SHEET_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	// Needed to ensure that the background color is correctly set in dark mode when opening the
	// dialog.
	InvalidateRect(propertySheet, nullptr, true);

	CenterWindow(parentWindow, propertySheet);

	return propertySheet;
}

int CALLBACK OptionsDialog::PropertySheetCallback(HWND dialog, UINT msg, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (msg)
	{
	case PSCB_INITIALIZED:
		OnPropertySheetInitialized(dialog);
		break;
	}

	return 0;
}

void OptionsDialog::OnPropertySheetInitialized(HWND dialog)
{
	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return;
	}

	darkModeHelper.AllowDarkModeForWindow(dialog, true);

	BOOL dark = TRUE;
	DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA compositionData = {
		DarkModeHelper::WCA_USEDARKMODECOLORS, &dark, sizeof(dark)
	};
	darkModeHelper.SetWindowCompositionAttribute(dialog, &compositionData);

	darkModeHelper.SetDarkModeForControl(GetDlgItem(dialog, IDOK));
	darkModeHelper.SetDarkModeForControl(GetDlgItem(dialog, IDCANCEL));
	darkModeHelper.SetDarkModeForControl(GetDlgItem(dialog, IDAPPLY));
}

PROPSHEETPAGE OptionsDialog::GeneratePropertySheetDefinition(
	const OptionsDialogSheetInfo &sheetInfo)
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

LRESULT CALLBACK OptionsDialog::PropSheetProcStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *optionsDialog = reinterpret_cast<OptionsDialog *>(dwRefData);
	return optionsDialog->PropSheetProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK OptionsDialog::PropSheetProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CTLCOLORDLG:
		return OnCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_DESTROY:
		OnDestroyDialog(hwnd);
		break;

	case WM_NCDESTROY:
		delete this;
		return 0;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void OptionsDialog::OnDestroyDialog(HWND dlg)
{
	HWND tabControl = PropSheet_GetTabControl(dlg);
	int index = TabCtrl_GetCurSel(tabControl);

	if (index != -1)
	{
		m_lastSelectedSheetIndex = index;
	}
}

INT_PTR CALLBACK OptionsDialog::GeneralSettingsProcStub(
	HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		auto *ppsp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
		optionsDialog = reinterpret_cast<OptionsDialog *>(ppsp->lParam);
	}
	break;
	}

	return optionsDialog->GeneralSettingsProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK OptionsDialog::GeneralSettingsProc(
	HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
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
		CheckDlgButton(hDlg, nIDButton, BST_CHECKED);

		CheckRadioButton(hDlg, IDC_OPTION_REPLACEEXPLORER_NONE, IDC_OPTION_REPLACEEXPLORER_ALL,
			REPLACE_EXPLORER_ENUM_CONTROL_ID_MAPPINGS.at(m_config->replaceExplorerMode));

		if (m_expp->GetSavePreferencesToXmlFile())
		{
			CheckDlgButton(hDlg, IDC_OPTION_XML, BST_CHECKED);
		}

		UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(hDlg);
		m_newTabDirectoryIcon =
			m_expp->GetIconResourceLoader()->LoadIconFromPNGForDpi(Icon::Folder, 16, 16, dpi);

		hButton = GetDlgItem(hDlg, IDC_DEFAULT_NEWTABDIR_BUTTON);
		SendMessage(hButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM) m_newTabDirectoryIcon.get());

		hEdit = GetDlgItem(hDlg, IDC_DEFAULT_NEWTABDIR_EDIT);
		DefaultSettingsSetNewTabDir(hEdit, m_config->defaultTabDirectory.c_str());

		AddIconThemes(hDlg);
		AddLanguages(hDlg);

		auto &darkModeHelper = DarkModeHelper::GetInstance();

		if (darkModeHelper.IsDarkModeEnabled())
		{
			darkModeHelper.SetDarkModeForControl(GetDlgItem(hDlg, IDC_DEFAULT_NEWTABDIR_BUTTON));
			darkModeHelper.SetDarkModeForComboBox(GetDlgItem(hDlg, IDC_OPTIONS_ICON_THEME));
			darkModeHelper.SetDarkModeForComboBox(GetDlgItem(hDlg, IDC_OPTIONS_LANGUAGE));

			m_checkboxControlIds.insert(IDC_OPTION_XML);
			m_radioButtonControlIds.insert({ IDC_STARTUP_PREVIOUSTABS, IDC_STARTUP_DEFAULTFOLDER,
				IDC_OPTION_REPLACEEXPLORER_NONE, IDC_OPTION_REPLACEEXPLORER_FILESYSTEM,
				IDC_OPTION_REPLACEEXPLORER_ALL });

			m_darkModeGroupBoxes.push_back(
				std::make_unique<DarkModeGroupBox>(GetDlgItem(hDlg, IDC_GROUP_STARTUP)));
			m_darkModeGroupBoxes.push_back(std::make_unique<DarkModeGroupBox>(
				GetDlgItem(hDlg, IDC_GROUP_DEFAULT_FILE_MANAGER)));
			m_darkModeGroupBoxes.push_back(
				std::make_unique<DarkModeGroupBox>(GetDlgItem(hDlg, IDC_GROUP_GENERAL_SETTINGS)));
		}
	}
	break;

	case WM_CTLCOLORDLG:
		return OnCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
		return OnCtlColor(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_COMMAND:
		if (HIWORD(wParam) != 0)
		{
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
			case CBN_SELCHANGE:
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;
			}
		}
		else
		{
			switch (LOWORD(wParam))
			{
			case IDC_STARTUP_PREVIOUSTABS:
			case IDC_STARTUP_DEFAULTFOLDER:
				if (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED)
				{
					PropSheet_Changed(GetParent(hDlg), hDlg);
				}
				break;

			case IDC_OPTION_REPLACEEXPLORER_NONE:
			case IDC_OPTION_REPLACEEXPLORER_FILESYSTEM:
			case IDC_OPTION_REPLACEEXPLORER_ALL:
			case IDC_OPTION_XML:
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;

			case IDC_DEFAULT_NEWTABDIR_BUTTON:
				OnDefaultSettingsNewTabDir(hDlg);
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;
			}
		}
		break;

	case WM_NOTIFY:
	{
		auto *nmhdr = reinterpret_cast<NMHDR *>(lParam);

		switch (nmhdr->code)
		{
		case NM_CUSTOMDRAW:
		{
			auto result = OnCustomDraw(reinterpret_cast<NMCUSTOMDRAW *>(lParam));
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
			return result;
		}
		break;

		case PSN_APPLY:
		{
			HWND hEdit;
			TCHAR szNewTabDir[MAX_PATH];
			TCHAR szVirtualParsingPath[MAX_PATH];
			ReplaceExplorerMode replaceExplorerMode = ReplaceExplorerMode::None;
			HRESULT hr;
			int iSel;

			if (IsDlgButtonChecked(hDlg, IDC_STARTUP_PREVIOUSTABS) == BST_CHECKED)
			{
				m_config->startupMode = StartupMode::PreviousTabs;
			}
			else if (IsDlgButtonChecked(hDlg, IDC_STARTUP_DEFAULTFOLDER) == BST_CHECKED)
			{
				m_config->startupMode = StartupMode::DefaultFolder;
			}

			if (IsDlgButtonChecked(hDlg, IDC_OPTION_REPLACEEXPLORER_NONE) == BST_CHECKED)
			{
				replaceExplorerMode = ReplaceExplorerMode::None;
			}
			else if (IsDlgButtonChecked(hDlg, IDC_OPTION_REPLACEEXPLORER_FILESYSTEM) == BST_CHECKED)
			{
				replaceExplorerMode = ReplaceExplorerMode::FileSystem;
			}
			else if (IsDlgButtonChecked(hDlg, IDC_OPTION_REPLACEEXPLORER_ALL) == BST_CHECKED)
			{
				replaceExplorerMode = ReplaceExplorerMode::All;
			}

			if (m_config->replaceExplorerMode != replaceExplorerMode)
			{
				OnReplaceExplorerSettingChanged(hDlg, replaceExplorerMode);
			}

			BOOL savePreferencesToXmlFile =
				(IsDlgButtonChecked(hDlg, IDC_OPTION_XML) == BST_CHECKED);
			m_expp->SetSavePreferencesToXmlFile(savePreferencesToXmlFile);

			hEdit = GetDlgItem(hDlg, IDC_DEFAULT_NEWTABDIR_EDIT);

			SendMessage(hEdit, WM_GETTEXT, SIZEOF_ARRAY(szNewTabDir), (LPARAM) szNewTabDir);

			/* The folder may be virtual, in which case, it needs
			to be decoded. */
			hr = DecodeFriendlyPath(
				szNewTabDir, szVirtualParsingPath, SIZEOF_ARRAY(szVirtualParsingPath));

			if (SUCCEEDED(hr))
			{
				m_config->defaultTabDirectory = szVirtualParsingPath;
			}
			else
			{
				m_config->defaultTabDirectory = szNewTabDir;
			}

			iSel = static_cast<int>(
				SendDlgItemMessage(hDlg, IDC_OPTIONS_ICON_THEME, CB_GETCURSEL, 0, 0));
			int iconThemeItemData = static_cast<int>(
				SendDlgItemMessage(hDlg, IDC_OPTIONS_ICON_THEME, CB_GETITEMDATA, iSel, 0));
			m_config->iconTheme = IconTheme::_from_integral(iconThemeItemData);

			iSel = static_cast<int>(
				SendDlgItemMessage(hDlg, IDC_OPTIONS_LANGUAGE, CB_GETCURSEL, 0, 0));

			int language = GetLanguageIDFromIndex(hDlg, iSel);
			m_config->language = language;

			m_expp->SaveAllSettings();
		}
		break;
		}
	}
	break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;
	}

	return 0;
}

void OptionsDialog::OnReplaceExplorerSettingChanged(
	HWND dialog, ReplaceExplorerMode updatedReplaceMode)
{
	bool settingChanged = UpdateReplaceExplorerSetting(dialog, updatedReplaceMode);

	if (!settingChanged)
	{
		// The default file manager setting was not changed, so reset the state of the file manager
		// radio buttons.
		CheckRadioButton(dialog, IDC_OPTION_REPLACEEXPLORER_NONE, IDC_OPTION_REPLACEEXPLORER_ALL,
			REPLACE_EXPLORER_ENUM_CONTROL_ID_MAPPINGS.at(m_config->replaceExplorerMode));
	}
}

bool OptionsDialog::UpdateReplaceExplorerSetting(
	HWND dialog, ReplaceExplorerMode updatedReplaceMode)
{
	if (updatedReplaceMode != ReplaceExplorerMode::None
		&& m_config->replaceExplorerMode == ReplaceExplorerMode::None)
	{
		std::wstring warningMessage =
			ResourceHelper::LoadString(m_instance, IDS_OPTIONS_DIALOG_REPLACE_EXPLORER_WARNING);

		int selectedButton = MessageBox(dialog, warningMessage.c_str(), NExplorerplusplus::APP_NAME,
			MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);

		if (selectedButton == IDNO)
		{
			return false;
		}
	}

	LSTATUS res = ERROR_SUCCESS;
	std::wstring menuText = ResourceHelper::LoadString(m_instance, IDS_OPEN_IN_EXPLORERPLUSPLUS);

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
		res = SetAsDefaultFileManagerFileSystem(
			SHELL_DEFAULT_INTERNAL_COMMAND_NAME, menuText.c_str());
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

			// Any trailing newlines are unnecessary, as they'll be added below when appropriate.
			boost::trim(finalSystemErrorMessage);
		}
		else
		{
			std::wstring errorCodeTemplate = ResourceHelper::LoadString(m_instance, IDS_ERROR_CODE);
			finalSystemErrorMessage = (boost::wformat(errorCodeTemplate) % res).str();
		}

		std::wstring errorMessage =
			ResourceHelper::LoadString(m_instance, IDS_ERROR_REPLACE_EXPLORER_SETTING) + L"\n\n"
			+ finalSystemErrorMessage;

		MessageBox(dialog, errorMessage.c_str(), NExplorerplusplus::APP_NAME, MB_ICONWARNING);

		return false;
	}
}

INT_PTR CALLBACK OptionsDialog::FilesFoldersProcStub(
	HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		auto *ppsp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
		optionsDialog = reinterpret_cast<OptionsDialog *>(ppsp->lParam);
	}
	break;
	}

	return optionsDialog->FilesFoldersProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK OptionsDialog::FilesFoldersProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		HWND hCBSize;

		if (m_config->globalFolderSettings.hideSystemFiles)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_SYSTEMFILES, BST_CHECKED);
		}

		if (!m_config->globalFolderSettings.showExtensions)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_EXTENSIONS, BST_CHECKED);
		}

		if (m_config->globalFolderSettings.hideLinkExtension)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_LINK, BST_CHECKED);
		}

		if (m_config->globalFolderSettings.insertSorted)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_INSERTSORTED, BST_CHECKED);
		}

		if (m_config->globalFolderSettings.oneClickActivate)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_SINGLECLICK, BST_CHECKED);
		}

		SetDlgItemInt(hDlg, IDC_OPTIONS_HOVER_TIME,
			m_config->globalFolderSettings.oneClickActivateHoverTime, FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_OPTIONS_HOVER_TIME),
			m_config->globalFolderSettings.oneClickActivate);
		EnableWindow(GetDlgItem(hDlg, IDC_LABEL_HOVER_TIME),
			m_config->globalFolderSettings.oneClickActivate);

		if (m_config->overwriteExistingFilesConfirmation)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION, BST_CHECKED);
		}

		if (m_config->playNavigationSound)
		{
			CheckDlgButton(hDlg, IDC_OPTIONS_PLAYNAVIGATIONSOUND, BST_CHECKED);
		}

		if (m_config->globalFolderSettings.showFolderSizes)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_FOLDERSIZES, BST_CHECKED);
		}

		if (m_config->globalFolderSettings.disableFolderSizesNetworkRemovable)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE, BST_CHECKED);
		}

		if (m_config->globalFolderSettings.forceSize)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_FORCESIZE, BST_CHECKED);
		}

		if (m_config->handleZipFiles)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_ZIPFILES, BST_CHECKED);
		}

		if (m_config->globalFolderSettings.showFriendlyDates)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_FRIENDLYDATES, BST_CHECKED);
		}

		if (m_config->showInfoTips)
		{
			CheckDlgButton(hDlg, IDC_OPTIONS_CHECK_SHOWINFOTIPS, BST_CHECKED);
		}

		if (m_config->infoTipType == InfoTipType::System)
		{
			CheckDlgButton(hDlg, IDC_OPTIONS_RADIO_SYSTEMINFOTIPS, BST_CHECKED);
		}
		else
		{
			CheckDlgButton(hDlg, IDC_OPTIONS_RADIO_CUSTOMINFOTIPS, BST_CHECKED);
		}

		hCBSize = GetDlgItem(hDlg, IDC_COMBO_FILESIZES);

		for (int i = 0; i < SIZEOF_ARRAY(FILE_SIZES); i++)
		{
			std::wstring fileSizeText =
				ResourceHelper::LoadString(m_instance, FILE_SIZES[i].StringID);
			SendMessage(hCBSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fileSizeText.c_str()));
			SendMessage(hCBSize, CB_SETITEMDATA, i, static_cast<LPARAM>(FILE_SIZES[i].sdf));

			if (FILE_SIZES[i].sdf == m_config->globalFolderSettings.sizeDisplayFormat)
			{
				SendMessage(hCBSize, CB_SETCURSEL, i, 0);
			}
		}

		EnableWindow(hCBSize, m_config->globalFolderSettings.forceSize);

		SetInfoTipWindowStates(hDlg);
		SetFolderSizeWindowState(hDlg);

		auto &darkModeHelper = DarkModeHelper::GetInstance();

		if (darkModeHelper.IsDarkModeEnabled())
		{
			darkModeHelper.SetDarkModeForComboBox(GetDlgItem(hDlg, IDC_COMBO_FILESIZES));

			m_checkboxControlIds.insert({ IDC_SETTINGS_CHECK_SYSTEMFILES,
				IDC_SETTINGS_CHECK_EXTENSIONS, IDC_SETTINGS_CHECK_LINK,
				IDC_SETTINGS_CHECK_INSERTSORTED, IDC_SETTINGS_CHECK_SINGLECLICK,
				IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION, IDC_OPTIONS_PLAYNAVIGATIONSOUND,
				IDC_SETTINGS_CHECK_FOLDERSIZES, IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE,
				IDC_SETTINGS_CHECK_FORCESIZE, IDC_SETTINGS_CHECK_ZIPFILES,
				IDC_SETTINGS_CHECK_FRIENDLYDATES, IDC_OPTIONS_CHECK_SHOWINFOTIPS });

			m_radioButtonControlIds.insert(
				{ IDC_OPTIONS_RADIO_SYSTEMINFOTIPS, IDC_OPTIONS_RADIO_CUSTOMINFOTIPS });
		}
	}
	break;

	case WM_CTLCOLORDLG:
		return OnCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
		return OnCtlColor(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_COMMAND:
		if (HIWORD(wParam) != 0)
		{
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;
			}
		}
		else
		{
			switch (LOWORD(wParam))
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
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;

			case IDC_SETTINGS_CHECK_FORCESIZE:
				EnableWindow(GetDlgItem(hDlg, IDC_COMBO_FILESIZES),
					IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;

			case IDC_OPTIONS_RADIO_SYSTEMINFOTIPS:
			case IDC_OPTIONS_RADIO_CUSTOMINFOTIPS:
				if (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED)
				{
					PropSheet_Changed(GetParent(hDlg), hDlg);
				}
				break;

			case IDC_OPTIONS_CHECK_SHOWINFOTIPS:
				SetInfoTipWindowStates(hDlg);
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;

			case IDC_SETTINGS_CHECK_FOLDERSIZES:
				SetFolderSizeWindowState(hDlg);
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;

			case IDC_SETTINGS_CHECK_SINGLECLICK:
				EnableWindow(GetDlgItem(hDlg, IDC_OPTIONS_HOVER_TIME),
					IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				EnableWindow(GetDlgItem(hDlg, IDC_LABEL_HOVER_TIME),
					IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;
			}
		}
		break;

	case WM_NOTIFY:
	{
		auto *nmhdr = reinterpret_cast<NMHDR *>(lParam);

		switch (nmhdr->code)
		{
		case NM_CUSTOMDRAW:
		{
			auto result = OnCustomDraw(reinterpret_cast<NMCUSTOMDRAW *>(lParam));
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
			return result;
		}
		break;

		case PSN_APPLY:
		{
			HWND hCBSize;
			int iSel;

			m_config->globalFolderSettings.hideSystemFiles =
				(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_SYSTEMFILES) == BST_CHECKED);

			m_config->globalFolderSettings.showExtensions =
				!(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_EXTENSIONS) == BST_CHECKED);

			m_config->globalFolderSettings.hideLinkExtension =
				(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_LINK) == BST_CHECKED);

			m_config->globalFolderSettings.insertSorted =
				(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_INSERTSORTED) == BST_CHECKED);

			m_config->globalFolderSettings.oneClickActivate =
				(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_SINGLECLICK) == BST_CHECKED);

			m_config->globalFolderSettings.oneClickActivateHoverTime =
				GetDlgItemInt(hDlg, IDC_OPTIONS_HOVER_TIME, nullptr, FALSE);

			m_config->overwriteExistingFilesConfirmation =
				(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION)
					== BST_CHECKED);

			m_config->playNavigationSound =
				(IsDlgButtonChecked(hDlg, IDC_OPTIONS_PLAYNAVIGATIONSOUND) == BST_CHECKED);

			m_config->globalFolderSettings.showFolderSizes =
				(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_FOLDERSIZES) == BST_CHECKED);

			m_config->globalFolderSettings.disableFolderSizesNetworkRemovable =
				(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE)
					== BST_CHECKED);

			m_config->globalFolderSettings.forceSize =
				(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_FORCESIZE) == BST_CHECKED);

			m_config->handleZipFiles =
				(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_ZIPFILES) == BST_CHECKED);

			m_config->globalFolderSettings.showFriendlyDates =
				(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_FRIENDLYDATES) == BST_CHECKED);

			m_config->showInfoTips =
				(IsDlgButtonChecked(hDlg, IDC_OPTIONS_CHECK_SHOWINFOTIPS) == BST_CHECKED);

			if (IsDlgButtonChecked(hDlg, IDC_OPTIONS_RADIO_SYSTEMINFOTIPS) == BST_CHECKED)
			{
				m_config->infoTipType = InfoTipType::System;
			}
			else
			{
				m_config->infoTipType = InfoTipType::Custom;
			}

			hCBSize = GetDlgItem(hDlg, IDC_COMBO_FILESIZES);

			iSel = (int) SendMessage(hCBSize, CB_GETCURSEL, 0, 0);
			m_config->globalFolderSettings.sizeDisplayFormat =
				(SizeDisplayFormat) SendMessage(hCBSize, CB_GETITEMDATA, iSel, 0);

			for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
			{
				tab->GetShellBrowser()->GetNavigationController()->Refresh();

				ListViewHelper::ActivateOneClickSelect(tab->GetShellBrowser()->GetListView(),
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
		EndDialog(hDlg, 0);
		break;
	}

	return 0;
}

INT_PTR CALLBACK OptionsDialog::WindowProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		auto *ppsp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
		optionsDialog = reinterpret_cast<OptionsDialog *>(ppsp->lParam);
	}
	break;
	}

	return optionsDialog->WindowProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK OptionsDialog::WindowProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		if (m_config->allowMultipleInstances)
		{
			CheckDlgButton(hDlg, IDC_OPTION_MULTIPLEINSTANCES, BST_CHECKED);
		}

		if (m_config->useLargeToolbarIcons.get())
		{
			CheckDlgButton(hDlg, IDC_OPTION_LARGETOOLBARICONS, BST_CHECKED);
		}

		if (m_config->alwaysShowTabBar.get())
		{
			CheckDlgButton(hDlg, IDC_OPTION_ALWAYSSHOWTABBAR, BST_CHECKED);
		}

		if (m_config->showTabBarAtBottom)
		{
			CheckDlgButton(hDlg, IDC_OPTION_SHOWTABBARATBOTTOM, BST_CHECKED);
		}

		if (m_config->showFilePreviews)
		{
			CheckDlgButton(hDlg, IDC_OPTION_FILEPREVIEWS, BST_CHECKED);
		}

		if (m_config->showFullTitlePath.get())
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_TITLEPATH, BST_CHECKED);
		}

		if (m_config->showUserNameInTitleBar.get())
		{
			CheckDlgButton(hDlg, IDC_OPTION_USERNAMEINTITLEBAR, BST_CHECKED);
		}

		if (m_config->showPrivilegeLevelInTitleBar.get())
		{
			CheckDlgButton(hDlg, IDC_OPTION_PRIVILEGELEVELINTITLEBAR, BST_CHECKED);
		}

		if (m_config->synchronizeTreeview)
		{
			CheckDlgButton(hDlg, IDC_OPTION_SYNCTREEVIEW, BST_CHECKED);
		}

		if (m_config->treeViewAutoExpandSelected)
		{
			CheckDlgButton(hDlg, IDC_OPTION_TREEVIEWSELECTIONEXPAND, BST_CHECKED);
		}

		if (!m_config->treeViewDelayEnabled)
		{
			CheckDlgButton(hDlg, IDC_OPTION_TREEVIEWDELAY, BST_CHECKED);
		}

		if (m_config->extendTabControl)
		{
			CheckDlgButton(hDlg, IDC_OPTION_EXTENDTABCONTROL, BST_CHECKED);
		}

		if (m_config->globalFolderSettings.showGridlines)
		{
			CheckDlgButton(hDlg, IDC_OPTION_GRIDLINES, BST_CHECKED);
		}

		if (m_config->checkBoxSelection)
		{
			CheckDlgButton(hDlg, IDC_OPTION_CHECKBOXSELECTION, BST_CHECKED);
		}

		if (m_config->useFullRowSelect)
		{
			CheckDlgButton(hDlg, IDC_OPTION_FULLROWSELECT, BST_CHECKED);
		}

		auto &darkModeHelper = DarkModeHelper::GetInstance();

		if (darkModeHelper.IsDarkModeEnabled())
		{
			m_checkboxControlIds.insert(
				{ IDC_OPTION_MULTIPLEINSTANCES, IDC_OPTION_LARGETOOLBARICONS,
					IDC_OPTION_ALWAYSSHOWTABBAR, IDC_OPTION_SHOWTABBARATBOTTOM,
					IDC_OPTION_EXTENDTABCONTROL, IDC_SETTINGS_CHECK_TITLEPATH,
					IDC_OPTION_USERNAMEINTITLEBAR, IDC_OPTION_PRIVILEGELEVELINTITLEBAR,
					IDC_OPTION_GRIDLINES, IDC_OPTION_CHECKBOXSELECTION, IDC_OPTION_FULLROWSELECT,
					IDC_OPTION_SYNCTREEVIEW, IDC_OPTION_TREEVIEWSELECTIONEXPAND,
					IDC_OPTION_TREEVIEWDELAY, IDC_OPTION_FILEPREVIEWS });

			m_darkModeGroupBoxes.push_back(
				std::make_unique<DarkModeGroupBox>(GetDlgItem(hDlg, IDC_GROUP_GENERAL)));
			m_darkModeGroupBoxes.push_back(
				std::make_unique<DarkModeGroupBox>(GetDlgItem(hDlg, IDC_GROUP_LISTVIEW)));
			m_darkModeGroupBoxes.push_back(
				std::make_unique<DarkModeGroupBox>(GetDlgItem(hDlg, IDC_GROUP_TREEVIEW)));
			m_darkModeGroupBoxes.push_back(
				std::make_unique<DarkModeGroupBox>(GetDlgItem(hDlg, IDC_GROUP_DISPLAY_WINDOW)));
		}
	}
	break;

	case WM_CTLCOLORDLG:
		return OnCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
		return OnCtlColor(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_COMMAND:
		switch (LOWORD(wParam))
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
			PropSheet_Changed(GetParent(hDlg), hDlg);
			break;
		}
		break;

	case WM_NOTIFY:
	{
		auto *nmhdr = reinterpret_cast<NMHDR *>(lParam);

		switch (nmhdr->code)
		{
		case NM_CUSTOMDRAW:
		{
			auto result = OnCustomDraw(reinterpret_cast<NMCUSTOMDRAW *>(lParam));
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
			return result;
		}
		break;

		case PSN_APPLY:
		{
			BOOL bCheckBoxSelection;

			m_config->allowMultipleInstances =
				(IsDlgButtonChecked(hDlg, IDC_OPTION_MULTIPLEINSTANCES) == BST_CHECKED);

			m_config->alwaysShowTabBar.set(
				IsDlgButtonChecked(hDlg, IDC_OPTION_ALWAYSSHOWTABBAR) == BST_CHECKED);

			m_config->showTabBarAtBottom =
				(IsDlgButtonChecked(hDlg, IDC_OPTION_SHOWTABBARATBOTTOM) == BST_CHECKED);

			m_config->showFilePreviews =
				(IsDlgButtonChecked(hDlg, IDC_OPTION_FILEPREVIEWS) == BST_CHECKED);

			m_config->showFullTitlePath.set(
				IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_TITLEPATH) == BST_CHECKED);

			m_config->showUserNameInTitleBar.set(
				IsDlgButtonChecked(hDlg, IDC_OPTION_USERNAMEINTITLEBAR) == BST_CHECKED);

			m_config->showPrivilegeLevelInTitleBar.set(
				IsDlgButtonChecked(hDlg, IDC_OPTION_PRIVILEGELEVELINTITLEBAR) == BST_CHECKED);

			m_config->synchronizeTreeview =
				(IsDlgButtonChecked(hDlg, IDC_OPTION_SYNCTREEVIEW) == BST_CHECKED);

			m_config->treeViewAutoExpandSelected =
				(IsDlgButtonChecked(hDlg, IDC_OPTION_TREEVIEWSELECTIONEXPAND) == BST_CHECKED);

			m_config->treeViewDelayEnabled =
				!(IsDlgButtonChecked(hDlg, IDC_OPTION_TREEVIEWDELAY) == BST_CHECKED);

			m_config->extendTabControl =
				(IsDlgButtonChecked(hDlg, IDC_OPTION_EXTENDTABCONTROL) == BST_CHECKED);

			m_config->globalFolderSettings.showGridlines =
				(IsDlgButtonChecked(hDlg, IDC_OPTION_GRIDLINES) == BST_CHECKED);

			bCheckBoxSelection =
				(IsDlgButtonChecked(hDlg, IDC_OPTION_CHECKBOXSELECTION) == BST_CHECKED);

			if (m_config->checkBoxSelection != bCheckBoxSelection)
			{
				for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
				{
					auto dwExtendedStyle =
						ListView_GetExtendedListViewStyle(tab->GetShellBrowser()->GetListView());

					if (bCheckBoxSelection)
					{
						dwExtendedStyle |= LVS_EX_CHECKBOXES;
					}
					else
					{
						dwExtendedStyle &= ~LVS_EX_CHECKBOXES;
					}

					ListView_SetExtendedListViewStyle(
						tab->GetShellBrowser()->GetListView(), dwExtendedStyle);
				}

				m_config->checkBoxSelection =
					(IsDlgButtonChecked(hDlg, IDC_OPTION_CHECKBOXSELECTION) == BST_CHECKED);
			}

			m_config->useFullRowSelect =
				(IsDlgButtonChecked(hDlg, IDC_OPTION_FULLROWSELECT) == BST_CHECKED);

			m_config->useLargeToolbarIcons.set(
				IsDlgButtonChecked(hDlg, IDC_OPTION_LARGETOOLBARICONS) == BST_CHECKED);

			for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
			{
				/* TODO: The tab should monitor for settings
				changes itself. */
				tab->GetShellBrowser()->OnGridlinesSettingChanged();

				ListViewHelper::AddRemoveExtendedStyle(tab->GetShellBrowser()->GetListView(),
					LVS_EX_FULLROWSELECT, m_config->useFullRowSelect);
			}

			m_expp->SaveAllSettings();
		}
		break;
		}
	}
	break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;
	}

	return 0;
}

INT_PTR CALLBACK OptionsDialog::TabSettingsProcStub(
	HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		auto *ppsp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
		optionsDialog = reinterpret_cast<OptionsDialog *>(ppsp->lParam);
	}
	break;
	}

	return optionsDialog->TabSettingsProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK OptionsDialog::TabSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		if (m_config->showTaskbarThumbnails)
		{
			CheckDlgButton(hDlg, IDC_TABS_TASKBARTHUMBNAILS, BST_CHECKED);
		}

		if (m_config->forceSameTabWidth.get())
		{
			CheckDlgButton(hDlg, IDC_TABS_SAMEWIDTH, BST_CHECKED);
		}

		if (m_config->confirmCloseTabs)
		{
			CheckDlgButton(hDlg, IDC_TABS_CLOSECONFIRMATION, BST_CHECKED);
		}

		if (m_config->openNewTabNextToCurrent)
		{
			CheckDlgButton(hDlg, IDC_TABS_OPENNEXTTOCURRENT, BST_CHECKED);
		}

		if (m_config->alwaysOpenNewTab)
		{
			CheckDlgButton(hDlg, IDC_SETTINGS_CHECK_ALWAYSNEWTAB, BST_CHECKED);
		}

		if (m_config->doubleClickTabClose)
		{
			CheckDlgButton(hDlg, IDC_TABS_DOUBLECLICKCLOSE, BST_CHECKED);
		}

		if (m_config->closeMainWindowOnTabClose)
		{
			CheckDlgButton(hDlg, IDC_TABS_CLOSEMAINWINDOW, BST_CHECKED);
		}

		auto &darkModeHelper = DarkModeHelper::GetInstance();

		if (darkModeHelper.IsDarkModeEnabled())
		{
			m_checkboxControlIds.insert(
				{ IDC_TABS_TASKBARTHUMBNAILS, IDC_TABS_SAMEWIDTH, IDC_TABS_CLOSECONFIRMATION,
					IDC_TABS_OPENNEXTTOCURRENT, IDC_SETTINGS_CHECK_ALWAYSNEWTAB,
					IDC_TABS_DOUBLECLICKCLOSE, IDC_TABS_CLOSEMAINWINDOW });
		}
	}
	break;

	case WM_CTLCOLORDLG:
		return OnCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
		return OnCtlColor(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_TABS_TASKBARTHUMBNAILS:
		case IDC_TABS_SAMEWIDTH:
		case IDC_TABS_CLOSECONFIRMATION:
		case IDC_TABS_OPENNEXTTOCURRENT:
		case IDC_SETTINGS_CHECK_ALWAYSNEWTAB:
		case IDC_TABS_DOUBLECLICKCLOSE:
		case IDC_TABS_CLOSEMAINWINDOW:
			PropSheet_Changed(GetParent(hDlg), hDlg);
			break;
		}
		break;

	case WM_NOTIFY:
	{
		auto *nmhdr = reinterpret_cast<NMHDR *>(lParam);

		switch (nmhdr->code)
		{
		case NM_CUSTOMDRAW:
		{
			auto result = OnCustomDraw(reinterpret_cast<NMCUSTOMDRAW *>(lParam));
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
			return result;
		}
		break;

		case PSN_APPLY:
		{
			m_config->showTaskbarThumbnails =
				(IsDlgButtonChecked(hDlg, IDC_TABS_TASKBARTHUMBNAILS) == BST_CHECKED);

			m_config->forceSameTabWidth.set(
				IsDlgButtonChecked(hDlg, IDC_TABS_SAMEWIDTH) == BST_CHECKED);

			m_config->confirmCloseTabs =
				(IsDlgButtonChecked(hDlg, IDC_TABS_CLOSECONFIRMATION) == BST_CHECKED);

			m_config->openNewTabNextToCurrent =
				(IsDlgButtonChecked(hDlg, IDC_TABS_OPENNEXTTOCURRENT) == BST_CHECKED);

			m_config->alwaysOpenNewTab =
				(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_ALWAYSNEWTAB) == BST_CHECKED);

			m_config->doubleClickTabClose =
				(IsDlgButtonChecked(hDlg, IDC_TABS_DOUBLECLICKCLOSE) == BST_CHECKED);

			m_config->closeMainWindowOnTabClose =
				(IsDlgButtonChecked(hDlg, IDC_TABS_CLOSEMAINWINDOW) == BST_CHECKED);

			m_expp->SaveAllSettings();
		}
		break;
		}
	}
	break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;
	}

	return 0;
}

INT_PTR CALLBACK OptionsDialog::DefaultSettingsProcStub(
	HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		auto *ppsp = reinterpret_cast<PROPSHEETPAGE *>(lParam);
		optionsDialog = reinterpret_cast<OptionsDialog *>(ppsp->lParam);
	}
	break;
	}

	return optionsDialog->DefaultSettingsProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK OptionsDialog::DefaultSettingsProc(
	HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		if (m_config->defaultFolderSettings.showHidden)
		{
			CheckDlgButton(hDlg, IDC_SHOWHIDDENGLOBAL, BST_CHECKED);
		}

		if (m_config->defaultFolderSettings.showInGroups)
		{
			CheckDlgButton(hDlg, IDC_SHOWINGROUPSGLOBAL, BST_CHECKED);
		}

		if (m_config->defaultFolderSettings.autoArrange)
		{
			CheckDlgButton(hDlg, IDC_AUTOARRANGEGLOBAL, BST_CHECKED);
		}

		if (m_config->defaultFolderSettings.sortAscending)
		{
			CheckDlgButton(hDlg, IDC_SORTASCENDINGGLOBAL, BST_CHECKED);
		}

		HWND hComboBox = GetDlgItem(hDlg, IDC_OPTIONS_DEFAULT_VIEW);
		int selectedIndex = -1;

		for (auto viewMode : VIEW_MODES)
		{
			int stringId = GetViewModeMenuStringId(viewMode);

			std::wstring viewModeText = ResourceHelper::LoadString(m_instance, stringId);

			int index = static_cast<int>(SendMessage(
				hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(viewModeText.c_str())));

			if (index != CB_ERR)
			{
				SendMessage(hComboBox, CB_SETITEMDATA, index, viewMode);
			}

			if (viewMode == m_config->defaultFolderSettings.viewMode)
			{
				selectedIndex = index;
			}
		}

		SendMessage(hComboBox, CB_SETCURSEL, selectedIndex, 0);

		auto &darkModeHelper = DarkModeHelper::GetInstance();

		if (darkModeHelper.IsDarkModeEnabled())
		{
			darkModeHelper.SetDarkModeForControl(GetDlgItem(hDlg, IDC_BUTTON_DEFAULTCOLUMNS));
			darkModeHelper.SetDarkModeForComboBox(GetDlgItem(hDlg, IDC_OPTIONS_DEFAULT_VIEW));

			m_checkboxControlIds.insert({ IDC_SHOWHIDDENGLOBAL, IDC_AUTOARRANGEGLOBAL,
				IDC_SHOWINGROUPSGLOBAL, IDC_SORTASCENDINGGLOBAL });
		}
	}
	break;

	case WM_CTLCOLORDLG:
		return OnCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
		return OnCtlColor(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_COMMAND:
		if (HIWORD(wParam) != 0)
		{
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;
			}
		}
		else
		{
			switch (LOWORD(wParam))
			{
			case IDC_SHOWHIDDENGLOBAL:
			case IDC_AUTOARRANGEGLOBAL:
			case IDC_SORTASCENDINGGLOBAL:
			case IDC_SHOWINGROUPSGLOBAL:
				PropSheet_Changed(GetParent(hDlg), hDlg);
				break;

			case IDC_BUTTON_DEFAULTCOLUMNS:
			{
				SetDefaultColumnsDialog setDefaultColumnsDialog(
					m_instance, hDlg, m_config->globalFolderSettings.folderColumns);
				setDefaultColumnsDialog.ShowModalDialog();
			}
			break;
			}
		}
		break;

	case WM_NOTIFY:
	{
		auto *nmhdr = reinterpret_cast<NMHDR *>(lParam);

		switch (nmhdr->code)
		{
		case NM_CUSTOMDRAW:
		{
			auto result = OnCustomDraw(reinterpret_cast<NMCUSTOMDRAW *>(lParam));
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
			return result;
		}
		break;

		case PSN_APPLY:
		{
			m_config->defaultFolderSettings.showHidden =
				(IsDlgButtonChecked(hDlg, IDC_SHOWHIDDENGLOBAL) == BST_CHECKED);

			m_config->defaultFolderSettings.showInGroups =
				(IsDlgButtonChecked(hDlg, IDC_SHOWINGROUPSGLOBAL) == BST_CHECKED);

			m_config->defaultFolderSettings.autoArrange =
				(IsDlgButtonChecked(hDlg, IDC_AUTOARRANGEGLOBAL) == BST_CHECKED);

			m_config->defaultFolderSettings.sortAscending =
				(IsDlgButtonChecked(hDlg, IDC_SORTASCENDINGGLOBAL) == BST_CHECKED);

			HWND hComboBox = GetDlgItem(hDlg, IDC_OPTIONS_DEFAULT_VIEW);
			int selectedIndex = static_cast<int>(SendMessage(hComboBox, CB_GETCURSEL, 0, 0));
			m_config->defaultFolderSettings.viewMode = ViewMode::_from_integral(
				static_cast<int>(SendMessage(hComboBox, CB_GETITEMDATA, selectedIndex, 0)));

			m_expp->SaveAllSettings();
		}
		break;
		}
	}
	break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;
	}

	return 0;
}

INT_PTR OptionsDialog::OnCtlColorDlg(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(hdc);

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return FALSE;
	}

	return reinterpret_cast<INT_PTR>(darkModeHelper.GetBackgroundBrush());
}

INT_PTR OptionsDialog::OnCtlColor(HWND hwnd, HDC hdc)
{
	UNREFERENCED_PARAMETER(hwnd);

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (!darkModeHelper.IsDarkModeEnabled())
	{
		return FALSE;
	}

	SetBkColor(hdc, DarkModeHelper::BACKGROUND_COLOR);
	SetTextColor(hdc, DarkModeHelper::TEXT_COLOR);

	return reinterpret_cast<INT_PTR>(darkModeHelper.GetBackgroundBrush());
}

INT_PTR OptionsDialog::OnCustomDraw(const NMCUSTOMDRAW *customDraw)
{
	bool isStoredCheckbox =
		(m_checkboxControlIds.count(static_cast<int>(customDraw->hdr.idFrom)) == 1);
	bool isStoredRadioButton =
		(m_radioButtonControlIds.count(static_cast<int>(customDraw->hdr.idFrom)) == 1);

	if (!isStoredCheckbox && !isStoredRadioButton)
	{
		return CDRF_DODEFAULT;
	}

	switch (customDraw->dwDrawStage)
	{
	case CDDS_PREPAINT:
		DrawButtonText(customDraw, isStoredCheckbox ? ButtonType::Checkbox : ButtonType::Radio);
		return CDRF_SKIPDEFAULT;
	}

	return CDRF_DODEFAULT;
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
	std::wstring helperText = ResourceHelper::LoadString(m_instance, IDS_DEFAULTSETTINGS_NEWTAB);

	GetDlgItemText(hDlg, IDC_DEFAULT_NEWTABDIR_EDIT, szNewTabDir, SIZEOF_ARRAY(szNewTabDir));

	hr = DecodeFriendlyPath(szNewTabDir, szVirtualParsingPath, SIZEOF_ARRAY(szVirtualParsingPath));

	if (SUCCEEDED(hr))
	{
		StringCchCopy(g_szNewTabDirectory, SIZEOF_ARRAY(g_szNewTabDirectory), szVirtualParsingPath);
	}
	else
	{
		StringCchCopy(g_szNewTabDirectory, SIZEOF_ARRAY(g_szNewTabDirectory), szNewTabDir);
	}

	bi.hwndOwner = hDlg;
	bi.pidlRoot = nullptr;
	bi.pszDisplayName = szDisplayName;
	bi.lpszTitle = helperText.c_str();
	bi.ulFlags = BIF_NEWDIALOGSTYLE;
	bi.lpfn = NewTabDirectoryBrowseCallbackProc;

	unique_pidl_absolute pidl(SHBrowseForFolder(&bi));

	if (pidl != nullptr)
	{
		hEdit = GetDlgItem(hDlg, IDC_DEFAULT_NEWTABDIR_EDIT);

		DefaultSettingsSetNewTabDir(hEdit, pidl.get());
	}
}

int CALLBACK NewTabDirectoryBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	UNREFERENCED_PARAMETER(lParam);
	UNREFERENCED_PARAMETER(lpData);

	switch (uMsg)
	{
	case BFFM_INITIALIZED:
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM) g_szNewTabDirectory);
		break;
	}

	return 0;
}

void OptionsDialog::DefaultSettingsSetNewTabDir(HWND hEdit, const TCHAR *szPath)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(szPath, nullptr, wil::out_param(pidl), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		DefaultSettingsSetNewTabDir(hEdit, pidl.get());
	}
}

void OptionsDialog::DefaultSettingsSetNewTabDir(HWND hEdit, PCIDLIST_ABSOLUTE pidl)
{
	SFGAOF attributes;
	DWORD uNameFlags;
	TCHAR szNewTabDir[MAX_PATH];

	attributes = SFGAO_FILESYSTEM;

	/* Check if the specified folder is real or virtual. */
	GetItemAttributes(pidl, &attributes);

	if (attributes & SFGAO_FILESYSTEM)
	{
		uNameFlags = SHGDN_FORPARSING;
	}
	else
	{
		uNameFlags = SHGDN_INFOLDER;
	}

	GetDisplayName(pidl, szNewTabDir, SIZEOF_ARRAY(szNewTabDir), uNameFlags);

	SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) szNewTabDir);
}

void OptionsDialog::AddIconThemes(HWND dlg)
{
	HWND iconThemeControl = GetDlgItem(dlg, IDC_OPTIONS_ICON_THEME);

	for (auto theme : IconTheme::_values())
	{
		UINT stringResourceId = GetIconThemeStringResourceId(theme);
		std::wstring iconThemeName = ResourceHelper::LoadString(m_instance, stringResourceId);

		int index = static_cast<int>(SendMessage(
			iconThemeControl, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(iconThemeName.c_str())));

		if (index == CB_ERR)
		{
			continue;
		}

		SendMessage(iconThemeControl, CB_SETITEMDATA, index, theme);

		if (theme == m_config->iconTheme)
		{
			SendMessage(iconThemeControl, CB_SETCURSEL, index, 0);
		}
	}
}

UINT GetIconThemeStringResourceId(IconTheme iconTheme)
{
	switch (iconTheme)
	{
	case IconTheme::Color:
		return IDS_ICON_THEME_COLOR;

	case IconTheme::FluentUi:
		return IDS_ICON_THEME_FLUENT_UI;

	case IconTheme::Windows10:
		return IDS_ICON_THEME_WINDOWS_10;

	default:
		throw std::runtime_error("IconTheme value not found");
	}
}

void OptionsDialog::AddLanguages(HWND hDlg)
{
	HWND hLanguageComboBox;
	WIN32_FIND_DATA wfd;
	HANDLE hFindFile;
	TCHAR szImageDirectory[MAX_PATH];
	TCHAR szNamePattern[MAX_PATH];
	WORD wLanguage;
	int iIndex = 1;
	int iSel = 0;

	hLanguageComboBox = GetDlgItem(hDlg, IDC_OPTIONS_LANGUAGE);

	/* English will always be added to the combox, and will
	always be the first item. */
	SendMessage(hLanguageComboBox, CB_ADDSTRING, 0, (LPARAM) _T("English"));
	SendMessage(hLanguageComboBox, CB_SETITEMDATA, 0, 9);

	GetProcessImageName(GetCurrentProcessId(), szImageDirectory, SIZEOF_ARRAY(szImageDirectory));
	PathRemoveFileSpec(szImageDirectory);
	StringCchCopy(szNamePattern, SIZEOF_ARRAY(szNamePattern), szImageDirectory);
	PathAppend(szNamePattern, NExplorerplusplus::LANGUAGE_DLL_FILENAME_PATTERN);

	hFindFile = FindFirstFile(szNamePattern, &wfd);

	/* Enumerate all the possible language DLL's. */
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			BOOL bRet = AddLanguageToComboBox(
				hLanguageComboBox, szImageDirectory, wfd.cFileName, &wLanguage);

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

BOOL OptionsDialog::AddLanguageToComboBox(
	HWND hComboBox, const TCHAR *szImageDirectory, const TCHAR *szFileName, WORD *pdwLanguage)
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

		int iRet = GetLocaleInfo(
			wLanguage, LOCALE_SNATIVELANGNAME, szLanguageName, SIZEOF_ARRAY(szLanguageName));

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

int OptionsDialog::GetLanguageIDFromIndex(HWND hDlg, int iIndex)
{
	HWND hComboBox;
	int iLanguage;

	hComboBox = GetDlgItem(hDlg, IDC_OPTIONS_LANGUAGE);

	iLanguage = (int) SendMessage(hComboBox, CB_GETITEMDATA, iIndex, 0);

	return iLanguage;
}

void OptionsDialog::SetInfoTipWindowStates(HWND hDlg)
{
	HWND hCheckSystemInfoTips;
	HWND hCheckCustomInfoTips;
	BOOL bEnable;

	hCheckSystemInfoTips = GetDlgItem(hDlg, IDC_OPTIONS_RADIO_SYSTEMINFOTIPS);
	hCheckCustomInfoTips = GetDlgItem(hDlg, IDC_OPTIONS_RADIO_CUSTOMINFOTIPS);

	bEnable = (IsDlgButtonChecked(hDlg, IDC_OPTIONS_CHECK_SHOWINFOTIPS) == BST_CHECKED);

	EnableWindow(hCheckSystemInfoTips, bEnable);
	EnableWindow(hCheckCustomInfoTips, bEnable);
}

void OptionsDialog::SetFolderSizeWindowState(HWND hDlg)
{
	HWND hFolderSizesNeworkRemovable;
	BOOL bEnable;

	hFolderSizesNeworkRemovable = GetDlgItem(hDlg, IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE);

	bEnable = (IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_FOLDERSIZES) == BST_CHECKED);

	EnableWindow(hFolderSizesNeworkRemovable, bEnable);
}