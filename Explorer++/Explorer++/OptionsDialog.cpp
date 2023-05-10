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
#include "Icon.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "SetDefaultColumnsDialog.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "TabContainer.h"
#include "ViewModeHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/RichEditHelper.h"
#include "../Helper/SetDefaultFileManager.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <boost/algorithm/string.hpp>
#include <boost/bimap.hpp>
#include <boost/format.hpp>
#include <boost/range/adaptor/map.hpp>
#include <unordered_map>

using namespace DarkModeButton;
using namespace DefaultFileManager;

int CALLBACK NewTabDirectoryBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
UINT GetSizeDisplayFormatStringResourceId(SizeDisplayFormat sizeDisplayFormat);
UINT GetIconSetStringResourceId(IconSet iconSet);
UINT GetThemeStringResourceId(Theme theme);

// clang-format off
const OptionsDialog::PageInfo OptionsDialog::SETTINGS_PAGES[] = {
	{IDD_OPTIONS_GENERAL, IDS_OPTIONS_GENERAL_TITLE, GeneralSettingsProcStub},
	{IDD_OPTIONS_APPEARANCE, IDS_OPTIONS_APPEARANCE_TITLE, AppearanceProcStub},
	{IDD_OPTIONS_FILES_FOLDERS, IDS_OPTIONS_FILES_FOLDERS_TITLE, FilesFoldersProcStub},
	{IDD_OPTIONS_WINDOW, IDS_OPTIONS_WINDOW_TITLE, WindowProcStub},
	{IDD_OPTIONS_TABS, IDS_OPTIONS_TABS_TITLE, TabSettingsProcStub},
	{IDD_OPTIONS_DEFAULT, IDS_OPTIONS_DEFAULT_TITLE, DefaultSettingsProcStub},
	{IDD_OPTIONS_ADVANCED, IDS_OPTIONS_ADVANCED_TITLE, AdvancedSettingsProcStub}
};
// clang-format on

const std::unordered_map<ReplaceExplorerMode, int> REPLACE_EXPLORER_ENUM_CONTROL_ID_MAPPINGS = {
	{ ReplaceExplorerMode::None, IDC_OPTION_REPLACEEXPLORER_NONE },
	{ ReplaceExplorerMode::FileSystem, IDC_OPTION_REPLACEEXPLORER_FILESYSTEM },
	{ ReplaceExplorerMode::All, IDC_OPTION_REPLACEEXPLORER_ALL }
};

#pragma warning(push)
#pragma warning(                                                                                   \
	disable : 4996 4834) // warning STL4010: Various members of std::allocator are
						 // deprecated in C++17,
						 // discarding return value of function with 'nodiscard' attribute

const boost::bimap<bool, std::wstring> BOOL_MAPPINGS =
	MakeBimap<bool, std::wstring>({ { true, L"true" }, { false, L"false" } });

#pragma warning(pop)

TCHAR g_szNewTabDirectory[MAX_PATH];

OptionsDialog::OptionsDialog(HINSTANCE resourceInstance, HWND parent,
	std::shared_ptr<Config> config, CoreInterface *coreInterface, TabContainer *tabContainer) :
	DarkModeDialogBase(resourceInstance, IDD_OPTIONS, parent, DialogSizingType::Both),
	m_config(config),
	m_resourceInstance(resourceInstance),
	m_coreInterface(coreInterface),
	m_tabContainer(tabContainer)
{
}

INT_PTR OptionsDialog::OnInitDialog()
{
	SetWindowTheme(GetDlgItem(m_hDlg, IDC_SETTINGS_PAGES_TREE), L"Explorer", nullptr);

	AllowDarkModeForTreeView(IDC_SETTINGS_PAGES_TREE);
	AllowDarkModeForControls({ IDAPPLY });

	SelectPage(m_lastSelectedPageIndex);

	// Focus the treeview by default.
	SendMessage(m_hDlg, WM_NEXTDLGCTL,
		reinterpret_cast<WPARAM>(GetDlgItem(m_hDlg, IDC_SETTINGS_PAGES_TREE)), true);

	CenterWindow(GetParent(m_hDlg), m_hDlg);

	m_initializationFinished = true;

	return FALSE;
}

void OptionsDialog::AddDynamicControls()
{
	AddSettingsPages();
}

wil::unique_hicon OptionsDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_coreInterface->GetIconResourceLoader()->LoadIconFromPNGAndScale(Icon::Options,
		iconWidth, iconHeight);
}

std::vector<ResizableDialogControl> OptionsDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_SETTINGS_PAGES_TREE), MovingType::None,
		SizingType::Vertical);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Both, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDCANCEL), MovingType::Both, SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDAPPLY), MovingType::Both, SizingType::None);

	for (HWND dialogPage : m_dialogMap | std::views::values)
	{
		controls.emplace_back(dialogPage, MovingType::None, SizingType::Both);
	}

	return controls;
}

void OptionsDialog::AddSettingsPages()
{
	int index = 0;

	for (auto &page : SETTINGS_PAGES)
	{
		AddSettingsPage(page.dialogResourceId, page.titleResourceId, index++, page.dialogProc,
			reinterpret_cast<LPARAM>(this));
	}
}

void OptionsDialog::AddSettingsPage(UINT dialogResourceId, UINT titleResourceId, int pageIndex,
	DLGPROC dialogProc, LPARAM dialogProcParam)
{
	HWND dialog = CreateDialogParam(GetResourceInstance(), MAKEINTRESOURCE(dialogResourceId),
		m_hDlg, dialogProc, dialogProcParam);

	auto treeView = GetDlgItem(m_hDlg, IDC_SETTINGS_PAGES_TREE);

	RECT treeViewRect;
	GetWindowRect(treeView, &treeViewRect);
	MapWindowPoints(HWND_DESKTOP, m_hDlg, reinterpret_cast<LPPOINT>(&treeViewRect), 2);

	SetWindowPos(dialog, nullptr, treeViewRect.right + TREEVIEW_PAGE_HORIZONTAL_SPACING,
		treeViewRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	auto title = ResourceHelper::LoadString(m_resourceInstance, titleResourceId);

	TVITEMEX treeViewItem;
	treeViewItem.mask = TVIF_TEXT | TVIF_PARAM;
	treeViewItem.pszText = title.data();
	treeViewItem.lParam = pageIndex;

	TVINSERTSTRUCT insertInfo;
	insertInfo.hParent = nullptr;
	insertInfo.hInsertAfter = TVI_LAST;
	insertInfo.itemex = treeViewItem;

	auto insertedItem = TreeView_InsertItem(treeView, &insertInfo);

	m_dialogMap.insert({ pageIndex, dialog });
	m_treeMap.insert({ pageIndex, insertedItem });
}

void OptionsDialog::SelectPage(int index)
{
	// This will trigger a TVN_SELCHANGED notification. The handler for that will take care of
	// updating the page visibility.
	auto treeView = GetDlgItem(m_hDlg, IDC_SETTINGS_PAGES_TREE);
	TreeView_SelectItem(treeView, m_treeMap.at(index));
}

INT_PTR OptionsDialog::OnNotify(NMHDR *nmhdr)
{
	switch (nmhdr->code)
	{
	case TVN_SELCHANGED:
		OnTreeViewSelectionChanged(reinterpret_cast<NMTREEVIEW *>(nmhdr));
		break;
	}

	return 0;
}

void OptionsDialog::OnTreeViewSelectionChanged(const NMTREEVIEW *changeInfo)
{
	if (m_currentPageIndex)
	{
		ShowWindow(m_dialogMap.at(*m_currentPageIndex), SW_HIDE);
	}

	int updatedPageIndex = static_cast<int>(changeInfo->itemNew.lParam);
	ShowWindow(m_dialogMap.at(updatedPageIndex), SW_SHOW);
	m_currentPageIndex = updatedPageIndex;
}

INT_PTR OptionsDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)
	{
		return HandleMenuOrAccelerator(wParam);
	}

	return 1;
}

LRESULT OptionsDialog::HandleMenuOrAccelerator(WPARAM wParam)
{
	switch (LOWORD(wParam))
	{
	case IDOK:
		OnOk();
		break;

	case IDAPPLY:
		OnApply();
		break;

	case IDCANCEL:
		OnCancel();
		break;
	}

	return 0;
}

void OptionsDialog::OnOk()
{
	OnApply();
	DestroyWindow(m_hDlg);
}

void OptionsDialog::OnApply()
{
	for (auto dialog : m_dialogMap | std::views::values)
	{
		SendMessage(dialog, WM_APP_SAVE_SETTINGS, 0, 0);
	}

	m_coreInterface->SaveAllSettings();

	EnableWindow(GetDlgItem(m_hDlg, IDAPPLY), false);
}

void OptionsDialog::OnCancel()
{
	DestroyWindow(m_hDlg);
}

INT_PTR OptionsDialog::OnClose()
{
	DestroyWindow(m_hDlg);
	return 0;
}

void OptionsDialog::OnSettingChanged()
{
	if (m_initializationFinished)
	{
		EnableWindow(GetDlgItem(m_hDlg, IDAPPLY), true);
	}
}

INT_PTR OptionsDialog::OnDestroy()
{
	m_lastSelectedPageIndex = *m_currentPageIndex;
	return 0;
}

INT_PTR OptionsDialog::OnNcDestroy()
{
	delete this;

	return 0;
}

INT_PTR CALLBACK OptionsDialog::GeneralSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		optionsDialog = reinterpret_cast<OptionsDialog *>(lParam);
		break;
	}

	return optionsDialog->GeneralSettingsProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK OptionsDialog::GeneralSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		InitializeResizeDialogHelperGeneral(hDlg);

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

		if (m_coreInterface->GetSavePreferencesToXmlFile())
		{
			CheckDlgButton(hDlg, IDC_OPTION_XML, BST_CHECKED);
		}

		UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(hDlg);
		m_newTabDirectoryIcon = m_coreInterface->GetIconResourceLoader()->LoadIconFromPNGForDpi(
			Icon::Folder, 16, 16, dpi);

		hButton = GetDlgItem(hDlg, IDC_DEFAULT_NEWTABDIR_BUTTON);
		SendMessage(hButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM) m_newTabDirectoryIcon.get());

		hEdit = GetDlgItem(hDlg, IDC_DEFAULT_NEWTABDIR_EDIT);
		DefaultSettingsSetNewTabDir(hEdit, m_config->defaultTabDirectory.c_str());

		AddLanguages(hDlg);

		auto &darkModeHelper = DarkModeHelper::GetInstance();

		if (darkModeHelper.IsDarkModeEnabled())
		{
			darkModeHelper.SetDarkModeForControl(GetDlgItem(hDlg, IDC_DEFAULT_NEWTABDIR_BUTTON));
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
		return OnPageCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

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
				OnSettingChanged();
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
					OnSettingChanged();
				}
				break;

			case IDC_OPTION_REPLACEEXPLORER_NONE:
			case IDC_OPTION_REPLACEEXPLORER_FILESYSTEM:
			case IDC_OPTION_REPLACEEXPLORER_ALL:
			case IDC_OPTION_XML:
				OnSettingChanged();
				break;

			case IDC_DEFAULT_NEWTABDIR_BUTTON:
				OnDefaultSettingsNewTabDir(hDlg);
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
		}
	}
	break;

	case WM_SIZE:
		m_resizableDialogHelperGeneral->UpdateControls(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_APP_SAVE_SETTINGS:
	{
		HWND hEdit;
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

		BOOL savePreferencesToXmlFile = (IsDlgButtonChecked(hDlg, IDC_OPTION_XML) == BST_CHECKED);
		m_coreInterface->SetSavePreferencesToXmlFile(savePreferencesToXmlFile);

		hEdit = GetDlgItem(hDlg, IDC_DEFAULT_NEWTABDIR_EDIT);

		std::wstring newTabDir = GetWindowString(hEdit);

		/* The folder may be virtual, in which case, it needs
		to be decoded. */
		std::wstring virtualParsingPath;
		hr = DecodeFriendlyPath(newTabDir, virtualParsingPath);

		if (SUCCEEDED(hr))
		{
			m_config->defaultTabDirectory = virtualParsingPath;
		}
		else
		{
			m_config->defaultTabDirectory = newTabDir;
		}

		iSel = static_cast<int>(SendDlgItemMessage(hDlg, IDC_OPTIONS_LANGUAGE, CB_GETCURSEL, 0, 0));

		int language = GetLanguageIDFromIndex(hDlg, iSel);
		m_config->language = language;
	}
	break;
	}

	return 0;
}

void OptionsDialog::InitializeResizeDialogHelperGeneral(HWND dialog)
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(dialog, IDC_GROUP_STARTUP), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_STARTUP_PREVIOUSTABS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_STARTUP_DEFAULTFOLDER), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_GROUP_DEFAULT_FILE_MANAGER), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_REPLACEEXPLORER_NONE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_REPLACEEXPLORER_FILESYSTEM),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_REPLACEEXPLORER_ALL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_GROUP_GENERAL_SETTINGS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_XML), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_STATIC_NEW_TAB_FOLDER), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_DEFAULT_NEWTABDIR_EDIT), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_DEFAULT_NEWTABDIR_BUTTON), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTIONS_LANGUAGE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_STATIC_RESTART_FOOTNOTE), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(dialog, IDC_STATIC_RESTART_NOTICE), MovingType::None,
		SizingType::Horizontal);
	m_resizableDialogHelperGeneral = std::make_unique<ResizableDialogHelper>(dialog, controls);
}

INT_PTR CALLBACK OptionsDialog::AppearanceProcStub(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		optionsDialog = reinterpret_cast<OptionsDialog *>(lParam);
		break;
	}

	return optionsDialog->AppearanceProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK OptionsDialog::AppearanceProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		InitializeResizeDialogHelperAppearance(hDlg);

		// Adding a tooltip to a static control works, provided that the SS_NOTIFY style is set.
		// However, the tooltip won't show up if the control is disabled.
		// Adding the tooltip based on the control rectangle, while leaving out the SS_NOTIFY style,
		// will work in both cases.
		AddTooltipForControl(m_tipWnd, GetDlgItem(hDlg, IDC_OPTIONS_THEME_LABEL),
			m_resourceInstance, IDS_OPTIONS_THEME_TOOLTIP, TooltipType::Rectangle);

		// These calls add a tooltip both to the combobox control and to the control rectangle. The
		// first tooltip will activate when the control is enabled, while the second will activate
		// when the control is disabled.
		AddTooltipForControl(m_tipWnd, GetDlgItem(hDlg, IDC_OPTIONS_THEME), m_resourceInstance,
			IDS_OPTIONS_THEME_TOOLTIP, TooltipType::Control);
		AddTooltipForControl(m_tipWnd, GetDlgItem(hDlg, IDC_OPTIONS_THEME), m_resourceInstance,
			IDS_OPTIONS_THEME_TOOLTIP, TooltipType::Rectangle);

		auto &darkModeHelper = DarkModeHelper::GetInstance();

		if (!darkModeHelper.IsDarkModeSupported())
		{
			EnableWindow(GetDlgItem(hDlg, IDC_OPTIONS_THEME_LABEL), false);
			EnableWindow(GetDlgItem(hDlg, IDC_OPTIONS_THEME), false);
		}

		std::vector<IconSet> iconSets(IconSet::_values().begin(), IconSet::_values().end());
		AddItemsToComboBox<IconSet>(GetDlgItem(hDlg, IDC_OPTIONS_ICON_SET), iconSets,
			m_config->iconSet, GetIconSetStringResourceId);

		std::vector<Theme> themes(Theme::_values().begin(), Theme::_values().end());
		AddItemsToComboBox<Theme>(GetDlgItem(hDlg, IDC_OPTIONS_THEME), themes, m_config->theme,
			GetThemeStringResourceId);

		if (darkModeHelper.IsDarkModeEnabled())
		{
			darkModeHelper.SetDarkModeForComboBox(GetDlgItem(hDlg, IDC_OPTIONS_ICON_SET));
			darkModeHelper.SetDarkModeForComboBox(GetDlgItem(hDlg, IDC_OPTIONS_THEME));
		}
	}
	break;

	case WM_CTLCOLORDLG:
		return OnPageCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORLISTBOX:
		return OnCtlColor(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_COMMAND:
		if (HIWORD(wParam) != 0)
		{
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE:
				OnSettingChanged();
				break;
			}
		}
		break;

	case WM_SIZE:
		m_resizableDialogHelperAppearance->UpdateControls(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_APP_SAVE_SETTINGS:
	{
		int selectedIndex =
			static_cast<int>(SendDlgItemMessage(hDlg, IDC_OPTIONS_ICON_SET, CB_GETCURSEL, 0, 0));
		int iconSetItemData = static_cast<int>(
			SendDlgItemMessage(hDlg, IDC_OPTIONS_ICON_SET, CB_GETITEMDATA, selectedIndex, 0));
		m_config->iconSet = IconSet::_from_integral(iconSetItemData);

		selectedIndex =
			static_cast<int>(SendDlgItemMessage(hDlg, IDC_OPTIONS_THEME, CB_GETCURSEL, 0, 0));
		int themeItemData = static_cast<int>(
			SendDlgItemMessage(hDlg, IDC_OPTIONS_THEME, CB_GETITEMDATA, selectedIndex, 0));
		m_config->theme = Theme::_from_integral(themeItemData);
	}
	break;
	}

	return 0;
}

void OptionsDialog::InitializeResizeDialogHelperAppearance(HWND dialog)
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTIONS_ICON_SET), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_STATIC_RESTART_FOOTNOTE_1), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTIONS_THEME), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_STATIC_RESTART_FOOTNOTE_2), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(dialog, IDC_STATIC_RESTART_NOTICE), MovingType::None,
		SizingType::Horizontal);
	m_resizableDialogHelperAppearance = std::make_unique<ResizableDialogHelper>(dialog, controls);
}

void OptionsDialog::OnReplaceExplorerSettingChanged(HWND dialog,
	ReplaceExplorerMode updatedReplaceMode)
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

bool OptionsDialog::UpdateReplaceExplorerSetting(HWND dialog,
	ReplaceExplorerMode updatedReplaceMode)
{
	if (updatedReplaceMode != ReplaceExplorerMode::None
		&& m_config->replaceExplorerMode == ReplaceExplorerMode::None)
	{
		std::wstring warningMessage = ResourceHelper::LoadString(m_resourceInstance,
			IDS_OPTIONS_DIALOG_REPLACE_EXPLORER_WARNING);

		int selectedButton = MessageBox(dialog, warningMessage.c_str(), NExplorerplusplus::APP_NAME,
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

			// Any trailing newlines are unnecessary, as they'll be added below when appropriate.
			boost::trim(finalSystemErrorMessage);
		}
		else
		{
			std::wstring errorCodeTemplate =
				ResourceHelper::LoadString(m_resourceInstance, IDS_ERROR_CODE);
			finalSystemErrorMessage = (boost::wformat(errorCodeTemplate) % res).str();
		}

		std::wstring errorMessage =
			ResourceHelper::LoadString(m_resourceInstance, IDS_ERROR_REPLACE_EXPLORER_SETTING)
			+ L"\n\n" + finalSystemErrorMessage;

		MessageBox(dialog, errorMessage.c_str(), NExplorerplusplus::APP_NAME, MB_ICONWARNING);

		return false;
	}
}

INT_PTR CALLBACK OptionsDialog::FilesFoldersProcStub(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		optionsDialog = reinterpret_cast<OptionsDialog *>(lParam);
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
		InitializeResizeDialogHelperFilesFolders(hDlg);

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

		if (m_config->globalFolderSettings.displayMixedFilesAndFolders)
		{
			CheckDlgButton(hDlg, IDC_DISPLAY_MIXED_FILES_AND_FOLDERS, BST_CHECKED);
		}

		if (m_config->globalFolderSettings.useNaturalSortOrder)
		{
			CheckDlgButton(hDlg, IDC_USE_NATURAL_SORT_ORDER, BST_CHECKED);
		}

		AddTooltipForControl(m_tipWnd, GetDlgItem(hDlg, IDC_USE_NATURAL_SORT_ORDER),
			m_resourceInstance, IDS_USE_NATURAL_SORT_ORDER_TOOLTIP);

		HWND fileSizesComboBox = GetDlgItem(hDlg, IDC_COMBO_FILESIZES);
		std::vector<SizeDisplayFormat> fileSizeOptions = { SizeDisplayFormat::Bytes,
			SizeDisplayFormat::KB, SizeDisplayFormat::MB, SizeDisplayFormat::GB,
			SizeDisplayFormat::TB, SizeDisplayFormat::PB };
		AddItemsToComboBox<SizeDisplayFormat>(fileSizesComboBox, fileSizeOptions,
			m_config->globalFolderSettings.sizeDisplayFormat, GetSizeDisplayFormatStringResourceId);

		EnableWindow(fileSizesComboBox, m_config->globalFolderSettings.forceSize);

		SetInfoTipWindowStates(hDlg);
		SetFolderSizeWindowState(hDlg);

		auto &darkModeHelper = DarkModeHelper::GetInstance();

		if (darkModeHelper.IsDarkModeEnabled())
		{
			darkModeHelper.SetDarkModeForComboBox(GetDlgItem(hDlg, IDC_COMBO_FILESIZES));

			m_checkboxControlIds.insert(
				{ IDC_SETTINGS_CHECK_SYSTEMFILES, IDC_SETTINGS_CHECK_EXTENSIONS,
					IDC_SETTINGS_CHECK_LINK, IDC_SETTINGS_CHECK_INSERTSORTED,
					IDC_SETTINGS_CHECK_SINGLECLICK, IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION,
					IDC_SETTINGS_CHECK_FOLDERSIZES, IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE,
					IDC_SETTINGS_CHECK_FORCESIZE, IDC_SETTINGS_CHECK_ZIPFILES,
					IDC_SETTINGS_CHECK_FRIENDLYDATES, IDC_OPTIONS_CHECK_SHOWINFOTIPS,
					IDC_DISPLAY_MIXED_FILES_AND_FOLDERS, IDC_USE_NATURAL_SORT_ORDER });

			m_radioButtonControlIds.insert(
				{ IDC_OPTIONS_RADIO_SYSTEMINFOTIPS, IDC_OPTIONS_RADIO_CUSTOMINFOTIPS });
		}
	}
	break;

	case WM_CTLCOLORDLG:
		return OnPageCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

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
				OnSettingChanged();
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
			case IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE:
			case IDC_SETTINGS_CHECK_ZIPFILES:
			case IDC_SETTINGS_CHECK_FRIENDLYDATES:
			case IDC_OPTIONS_HOVER_TIME:
			case IDC_DISPLAY_MIXED_FILES_AND_FOLDERS:
			case IDC_USE_NATURAL_SORT_ORDER:
				OnSettingChanged();
				break;

			case IDC_SETTINGS_CHECK_FORCESIZE:
				EnableWindow(GetDlgItem(hDlg, IDC_COMBO_FILESIZES),
					IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				OnSettingChanged();
				break;

			case IDC_OPTIONS_RADIO_SYSTEMINFOTIPS:
			case IDC_OPTIONS_RADIO_CUSTOMINFOTIPS:
				if (IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED)
				{
					OnSettingChanged();
				}
				break;

			case IDC_OPTIONS_CHECK_SHOWINFOTIPS:
				SetInfoTipWindowStates(hDlg);
				OnSettingChanged();
				break;

			case IDC_SETTINGS_CHECK_FOLDERSIZES:
				SetFolderSizeWindowState(hDlg);
				OnSettingChanged();
				break;

			case IDC_SETTINGS_CHECK_SINGLECLICK:
				EnableWindow(GetDlgItem(hDlg, IDC_OPTIONS_HOVER_TIME),
					IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				EnableWindow(GetDlgItem(hDlg, IDC_LABEL_HOVER_TIME),
					IsDlgButtonChecked(hDlg, LOWORD(wParam)) == BST_CHECKED);
				OnSettingChanged();
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
		}
	}
	break;

	case WM_SIZE:
		m_resizableDialogHelperFilesFolders->UpdateControls(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_APP_SAVE_SETTINGS:
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
			(IsDlgButtonChecked(hDlg, IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION) == BST_CHECKED);

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

		m_config->globalFolderSettings.displayMixedFilesAndFolders =
			(IsDlgButtonChecked(hDlg, IDC_DISPLAY_MIXED_FILES_AND_FOLDERS) == BST_CHECKED);

		m_config->globalFolderSettings.useNaturalSortOrder =
			(IsDlgButtonChecked(hDlg, IDC_USE_NATURAL_SORT_ORDER) == BST_CHECKED);

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
	}
	break;
	}

	return 0;
}

void OptionsDialog::InitializeResizeDialogHelperFilesFolders(HWND dialog)
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_SYSTEMFILES), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_EXTENSIONS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_LINK), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_INSERTSORTED), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_SINGLECLICK), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_EXISTINGFILESCONFIRMATION),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_FOLDERSIZES), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_FOLDERSIZESNETWORKREMOVABLE),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_ZIPFILES), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_FRIENDLYDATES), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTIONS_CHECK_SHOWINFOTIPS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTIONS_RADIO_SYSTEMINFOTIPS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTIONS_RADIO_CUSTOMINFOTIPS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_DISPLAY_MIXED_FILES_AND_FOLDERS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_USE_NATURAL_SORT_ORDER), MovingType::None,
		SizingType::Horizontal);
	m_resizableDialogHelperFilesFolders = std::make_unique<ResizableDialogHelper>(dialog, controls);
}

INT_PTR CALLBACK OptionsDialog::WindowProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		optionsDialog = reinterpret_cast<OptionsDialog *>(lParam);
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
		InitializeResizeDialogHelperWindow(hDlg);

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

		if (m_config->showTabBarAtBottom.get())
		{
			CheckDlgButton(hDlg, IDC_OPTION_SHOWTABBARATBOTTOM, BST_CHECKED);
		}

		if (m_config->extendTabControl.get())
		{
			CheckDlgButton(hDlg, IDC_OPTION_EXTENDTABCONTROL, BST_CHECKED);
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
				std::make_unique<DarkModeGroupBox>(GetDlgItem(hDlg, IDC_GROUP_MAIN_PANE)));
			m_darkModeGroupBoxes.push_back(
				std::make_unique<DarkModeGroupBox>(GetDlgItem(hDlg, IDC_GROUP_NAVIGATION_PANE)));
			m_darkModeGroupBoxes.push_back(
				std::make_unique<DarkModeGroupBox>(GetDlgItem(hDlg, IDC_GROUP_DISPLAY_WINDOW)));
		}
	}
	break;

	case WM_CTLCOLORDLG:
		return OnPageCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

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
			OnSettingChanged();
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
		}
	}
	break;

	case WM_SIZE:
		m_resizableDialogHelperWindow->UpdateControls(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_APP_SAVE_SETTINGS:
	{
		BOOL bCheckBoxSelection;

		m_config->allowMultipleInstances =
			(IsDlgButtonChecked(hDlg, IDC_OPTION_MULTIPLEINSTANCES) == BST_CHECKED);

		m_config->alwaysShowTabBar.set(
			IsDlgButtonChecked(hDlg, IDC_OPTION_ALWAYSSHOWTABBAR) == BST_CHECKED);

		m_config->showTabBarAtBottom.set(
			(IsDlgButtonChecked(hDlg, IDC_OPTION_SHOWTABBARATBOTTOM) == BST_CHECKED));

		m_config->extendTabControl.set(
			(IsDlgButtonChecked(hDlg, IDC_OPTION_EXTENDTABCONTROL) == BST_CHECKED));

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

				ListView_SetExtendedListViewStyle(tab->GetShellBrowser()->GetListView(),
					dwExtendedStyle);
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
	}
	break;
	}

	return 0;
}

void OptionsDialog::InitializeResizeDialogHelperWindow(HWND dialog)
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(dialog, IDC_GROUP_GENERAL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_MULTIPLEINSTANCES), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_LARGETOOLBARICONS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_ALWAYSSHOWTABBAR), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_SHOWTABBARATBOTTOM), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_EXTENDTABCONTROL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_TITLEPATH), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_USERNAMEINTITLEBAR), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_PRIVILEGELEVELINTITLEBAR), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_GROUP_MAIN_PANE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_GRIDLINES), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_CHECKBOXSELECTION), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_FULLROWSELECT), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_GROUP_NAVIGATION_PANE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_SYNCTREEVIEW), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_TREEVIEWSELECTIONEXPAND), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_TREEVIEWDELAY), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_GROUP_DISPLAY_WINDOW), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_OPTION_FILEPREVIEWS), MovingType::None,
		SizingType::Horizontal);
	m_resizableDialogHelperWindow = std::make_unique<ResizableDialogHelper>(dialog, controls);
}

INT_PTR CALLBACK OptionsDialog::TabSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		optionsDialog = reinterpret_cast<OptionsDialog *>(lParam);
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
		InitializeResizeDialogHelperTabs(hDlg);

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
		return OnPageCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

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
			OnSettingChanged();
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
		}
	}
	break;

	case WM_SIZE:
		m_resizableDialogHelperTabs->UpdateControls(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_APP_SAVE_SETTINGS:
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
	}
	break;
	}

	return 0;
}

void OptionsDialog::InitializeResizeDialogHelperTabs(HWND dialog)
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(dialog, IDC_TABS_TASKBARTHUMBNAILS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_TABS_SAMEWIDTH), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_TABS_CLOSECONFIRMATION), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_TABS_OPENNEXTTOCURRENT), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SETTINGS_CHECK_ALWAYSNEWTAB), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_TABS_DOUBLECLICKCLOSE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_TABS_CLOSEMAINWINDOW), MovingType::None,
		SizingType::Horizontal);
	m_resizableDialogHelperTabs = std::make_unique<ResizableDialogHelper>(dialog, controls);
}

INT_PTR CALLBACK OptionsDialog::DefaultSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		optionsDialog = reinterpret_cast<OptionsDialog *>(lParam);
		break;
	}

	return optionsDialog->DefaultSettingsProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK OptionsDialog::DefaultSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		InitializeResizeDialogHelperDefaultSettings(hDlg);

		if (m_config->defaultFolderSettings.showHidden)
		{
			CheckDlgButton(hDlg, IDC_SHOWHIDDENGLOBAL, BST_CHECKED);
		}

		if (m_config->defaultFolderSettings.autoArrange)
		{
			CheckDlgButton(hDlg, IDC_AUTOARRANGEGLOBAL, BST_CHECKED);
		}

		if (m_config->defaultFolderSettings.sortDirection == +SortDirection::Ascending)
		{
			CheckDlgButton(hDlg, IDC_SORTASCENDINGGLOBAL, BST_CHECKED);
		}

		if (m_config->defaultFolderSettings.showInGroups)
		{
			CheckDlgButton(hDlg, IDC_SHOWINGROUPSGLOBAL, BST_CHECKED);
		}

		if (m_config->defaultFolderSettings.groupSortDirection == +SortDirection::Ascending)
		{
			CheckDlgButton(hDlg, IDC_GROUP_SORT_ASCENDING_GLOBAL, BST_CHECKED);
		}

		std::vector<ViewMode> viewModes(VIEW_MODES.begin(), VIEW_MODES.end());
		AddItemsToComboBox<ViewMode>(GetDlgItem(hDlg, IDC_OPTIONS_DEFAULT_VIEW), viewModes,
			m_config->defaultFolderSettings.viewMode, GetViewModeMenuStringId);

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
		return OnPageCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

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
				OnSettingChanged();
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
			case IDC_GROUP_SORT_ASCENDING_GLOBAL:
				OnSettingChanged();
				break;

			case IDC_BUTTON_DEFAULTCOLUMNS:
			{
				SetDefaultColumnsDialog setDefaultColumnsDialog(m_resourceInstance, hDlg,
					m_config->globalFolderSettings.folderColumns);
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
		}
	}
	break;

	case WM_SIZE:
		m_resizableDialogHelperDefaultSettings->UpdateControls(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_APP_SAVE_SETTINGS:
	{
		m_config->defaultFolderSettings.showHidden =
			(IsDlgButtonChecked(hDlg, IDC_SHOWHIDDENGLOBAL) == BST_CHECKED);

		m_config->defaultFolderSettings.autoArrange =
			(IsDlgButtonChecked(hDlg, IDC_AUTOARRANGEGLOBAL) == BST_CHECKED);

		bool sortAscending = (IsDlgButtonChecked(hDlg, IDC_SORTASCENDINGGLOBAL) == BST_CHECKED);
		m_config->defaultFolderSettings.sortDirection =
			sortAscending ? SortDirection::Ascending : SortDirection::Descending;

		m_config->defaultFolderSettings.showInGroups =
			(IsDlgButtonChecked(hDlg, IDC_SHOWINGROUPSGLOBAL) == BST_CHECKED);

		bool groupSortAscending =
			(IsDlgButtonChecked(hDlg, IDC_GROUP_SORT_ASCENDING_GLOBAL) == BST_CHECKED);
		m_config->defaultFolderSettings.groupSortDirection =
			groupSortAscending ? SortDirection::Ascending : SortDirection::Descending;

		HWND hComboBox = GetDlgItem(hDlg, IDC_OPTIONS_DEFAULT_VIEW);
		int selectedIndex = static_cast<int>(SendMessage(hComboBox, CB_GETCURSEL, 0, 0));
		m_config->defaultFolderSettings.viewMode = ViewMode::_from_integral(
			static_cast<int>(SendMessage(hComboBox, CB_GETITEMDATA, selectedIndex, 0)));
	}
	break;
	}

	return 0;
}

void OptionsDialog::InitializeResizeDialogHelperDefaultSettings(HWND dialog)
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(dialog, IDC_STATIC_DEFAULT_SETTINGS_NOTICE), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SHOWHIDDENGLOBAL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_AUTOARRANGEGLOBAL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SHOWINGROUPSGLOBAL), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_SORTASCENDINGGLOBAL), MovingType::None,
		SizingType::Horizontal);
	m_resizableDialogHelperDefaultSettings =
		std::make_unique<ResizableDialogHelper>(dialog, controls);
}

INT_PTR CALLBACK OptionsDialog::AdvancedSettingsProcStub(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	static OptionsDialog *optionsDialog;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		optionsDialog = reinterpret_cast<OptionsDialog *>(lParam);
		break;
	}

	return optionsDialog->AdvancedSettingsProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK OptionsDialog::AdvancedSettingsProc(HWND hDlg, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		InitializeResizeDialogHelperAdvanced(hDlg);

		HWND listView = GetDlgItem(hDlg, IDC_ADVANCED_OPTIONS);

		ListView_SetExtendedListViewStyle(listView,
			LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

		m_advancedOptionsListViewSubclass = std::make_unique<WindowSubclassWrapper>(listView,
			std::bind_front(&OptionsDialog::AdvancedOptionsListViewWndProc, this));

		SendDlgItemMessage(hDlg, IDC_ADVANCED_OPTION_DESCRIPTION, EM_AUTOURLDETECT, AURL_ENABLEURL,
			NULL);
		SendDlgItemMessage(hDlg, IDC_ADVANCED_OPTION_DESCRIPTION, EM_SETEVENTMASK, 0, ENM_LINK);

		auto &darkModeHelper = DarkModeHelper::GetInstance();

		if (darkModeHelper.IsDarkModeEnabled())
		{
			darkModeHelper.SetListViewDarkModeColors(listView);

			SendDlgItemMessage(hDlg, IDC_ADVANCED_OPTION_DESCRIPTION, EM_SETBKGNDCOLOR, 0,
				DarkModeHelper::BACKGROUND_COLOR);

			CHARFORMAT charFormat;
			charFormat.cbSize = sizeof(charFormat);
			charFormat.dwMask = CFM_COLOR;
			charFormat.crTextColor = DarkModeHelper::TEXT_COLOR;
			charFormat.dwEffects = 0;
			SendDlgItemMessage(hDlg, IDC_ADVANCED_OPTION_DESCRIPTION, EM_SETCHARFORMAT, SCF_ALL,
				reinterpret_cast<LPARAM>(&charFormat));
		}
		else
		{
			SetWindowTheme(listView, L"Explorer", nullptr);
		}

		std::wstring valueColumnText =
			ResourceHelper::LoadString(m_resourceInstance, IDS_ADVANCED_OPTION_VALUE);

		LV_COLUMN lvColumn;
		lvColumn.mask = LVCF_TEXT;
		lvColumn.pszText = valueColumnText.data();
		ListView_InsertColumn(listView, 0, &lvColumn);

		std::wstring optionColumnText =
			ResourceHelper::LoadString(m_resourceInstance, IDS_ADVANCED_OPTION);

		lvColumn.mask = LVCF_TEXT;
		lvColumn.pszText = optionColumnText.data();
		ListView_InsertColumn(listView, 1, &lvColumn);

		ListView_SetColumnWidth(listView, 0, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(listView, 1, LVSCW_AUTOSIZE_USEHEADER);

		int orderArray[] = { 1, 0 };
		ListView_SetColumnOrderArray(listView, SIZEOF_ARRAY(orderArray), orderArray);

		m_advancedOptions = InitializeAdvancedOptions();

		std::sort(m_advancedOptions.begin(), m_advancedOptions.end(),
			[](const AdvancedOption &option1, const AdvancedOption &option2)
			{ return option1.name < option2.name; });

		InsertAdvancedOptionsIntoListView(hDlg);
	}
	break;

	case WM_CTLCOLORDLG:
		return OnPageCtlColorDlg(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
		return OnCtlColor(reinterpret_cast<HWND>(lParam), reinterpret_cast<HDC>(wParam));

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR *>(lParam)->code)
		{
		case NM_DBLCLK:
		{
			const auto *info = reinterpret_cast<NMITEMACTIVATE *>(lParam);

			if (info->iItem != -1)
			{
				ListView_EditLabel(info->hdr.hwndFrom, info->iItem);
			}
		}
		break;

		case LVN_ITEMCHANGED:
		{
			auto info = reinterpret_cast<NMLISTVIEW *>(lParam);

			if (WI_IsFlagClear(info->uOldState, LVIS_SELECTED)
				&& WI_IsFlagSet(info->uNewState, LVIS_SELECTED))
			{
				auto *option = reinterpret_cast<AdvancedOption *>(info->lParam);
				SetDlgItemText(hDlg, IDC_ADVANCED_OPTION_DESCRIPTION, option->description.c_str());
			}
			else if (WI_IsFlagSet(info->uOldState, LVIS_SELECTED)
				&& WI_IsFlagClear(info->uNewState, LVIS_SELECTED))
			{
				// Since only a single item can be selected, if an item has been deselected, it
				// means that there's not currently any item selected (otherwise there would have
				// previously been two items selected).
				SetDlgItemText(hDlg, IDC_ADVANCED_OPTION_DESCRIPTION, L"");
			}
		}
		break;

		case LVN_ENDLABELEDIT:
		{
			auto *info = reinterpret_cast<NMLVDISPINFO *>(lParam);

			if (info->item.pszText == nullptr)
			{
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
				return FALSE;
			}

			if (lstrlen(info->item.pszText) == 0)
			{
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
				return FALSE;
			}

			auto *option = GetAdvancedOptionByIndex(hDlg, info->item.iItem);
			bool validValue = false;

			switch (option->type)
			{
			case AdvancedOptionType::Boolean:
			{
				auto itr = BOOL_MAPPINGS.right.find(info->item.pszText);

				if (itr != BOOL_MAPPINGS.right.end())
				{
					validValue = true;
				}
			}
			break;
			}

			if (!validValue)
			{
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
				return FALSE;
			}

			OnSettingChanged();

			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
			return TRUE;
		}
		break;

		case EN_LINK:
		{
			const auto *linkNotificationDetails = reinterpret_cast<ENLINK *>(lParam);

			if (linkNotificationDetails->nmhdr.hwndFrom
					== GetDlgItem(hDlg, IDC_ADVANCED_OPTION_DESCRIPTION)
				&& linkNotificationDetails->msg == WM_LBUTTONUP)
			{
				std::wstring text = GetRichEditLinkText(linkNotificationDetails);
				ShellExecute(nullptr, L"open", text.c_str(), nullptr, nullptr, SW_SHOW);
				return 1;
			}
		}
		break;
		}
		break;

	case WM_SIZE:
		m_resizableDialogHelperAdvanced->UpdateControls(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_APP_SAVE_SETTINGS:
	{
		HWND listView = GetDlgItem(hDlg, IDC_ADVANCED_OPTIONS);
		int numItems = ListView_GetItemCount(listView);

		for (int i = 0; i < numItems; i++)
		{
			TCHAR text[256];
			ListView_GetItemText(listView, i, 0, text, SIZEOF_ARRAY(text));

			auto &option = m_advancedOptions[i];

			switch (option.type)
			{
			case AdvancedOptionType::Boolean:
			{
				// Values are validated when editing, so the current value should always be
				// valid.
				bool newValue = BOOL_MAPPINGS.right.at(text);
				SetBooleanConfigValue(option.id, newValue);
			}
			break;
			}
		}
	}
	break;

	case WM_DESTROY:
		m_advancedOptionsListViewSubclass.reset();
		break;
	}

	return 0;
}

void OptionsDialog::InitializeResizeDialogHelperAdvanced(HWND dialog)
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(dialog, IDC_ADVANCED_OPTIONS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_STATIC_ADVANCED_OPTION_DESCRIPTION),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(dialog, IDC_ADVANCED_OPTION_DESCRIPTION), MovingType::None,
		SizingType::Horizontal);
	m_resizableDialogHelperAdvanced = std::make_unique<ResizableDialogHelper>(dialog, controls);
}

LRESULT CALLBACK OptionsDialog::AdvancedOptionsListViewWndProc(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == ListView_GetHeader(hwnd))
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_CUSTOMDRAW:
			{
				if (DarkModeHelper::GetInstance().IsDarkModeEnabled())
				{
					auto *customDraw = reinterpret_cast<NMCUSTOMDRAW *>(lParam);

					switch (customDraw->dwDrawStage)
					{
					case CDDS_PREPAINT:
						return CDRF_NOTIFYITEMDRAW;

					case CDDS_ITEMPREPAINT:
						SetTextColor(customDraw->hdc, DarkModeHelper::TEXT_COLOR);
						return CDRF_NEWFONT;
					}
				}
			}
			break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

std::vector<OptionsDialog::AdvancedOption> OptionsDialog::InitializeAdvancedOptions()
{
	std::vector<AdvancedOption> advancedOptions;

	AdvancedOption option;
	option.id = AdvancedOptionId::CheckSystemIsPinnedToNameSpaceTree;
	option.name = ResourceHelper::LoadString(m_resourceInstance,
		IDS_ADVANCED_OPTION_CHECK_PINNED_TO_NAMESPACE_TREE_NAME);
	option.type = AdvancedOptionType::Boolean;
	option.description = ResourceHelper::LoadString(m_resourceInstance,
		IDS_ADVANCED_OPTION_CHECK_PINNED_TO_NAMESPACE_TREE_DESCRIPTION);
	advancedOptions.push_back(option);

	option.id = AdvancedOptionId::OpenTabsInForeground;
	option.name = ResourceHelper::LoadString(m_resourceInstance,
		IDS_ADVANCED_OPTION_OPEN_TABS_IN_FOREGROUND_NAME);
	option.type = AdvancedOptionType::Boolean;
	option.description = ResourceHelper::LoadString(m_resourceInstance,
		IDS_ADVANCED_OPTION_OPEN_TABS_IN_FOREGROUND_DESCRIPTION);
	advancedOptions.push_back(option);

	option.id = AdvancedOptionId::GoUpOnDoubleClick;
	option.name = ResourceHelper::LoadString(m_resourceInstance,
		IDS_ADVANCED_OPTION_GO_UP_ON_DOUBLE_CLICK_NAME);
	option.type = AdvancedOptionType::Boolean;
	option.description = {};
	advancedOptions.push_back(option);

	return advancedOptions;
}

void OptionsDialog::InsertAdvancedOptionsIntoListView(HWND dlg)
{
	HWND listView = GetDlgItem(dlg, IDC_ADVANCED_OPTIONS);
	int index = 0;

	for (auto &option : m_advancedOptions)
	{
		LVITEM item;
		item.mask = LVIF_PARAM;
		item.iItem = index++;
		item.iSubItem = 0;
		item.lParam = reinterpret_cast<LPARAM>(&option);
		int finalIndex = ListView_InsertItem(listView, &item);

		if (finalIndex != -1)
		{
			std::wstring value;

			switch (option.type)
			{
			case AdvancedOptionType::Boolean:
				bool booleanValue = GetBooleanConfigValue(option.id);
				value = BOOL_MAPPINGS.left.at(booleanValue);
				break;
			}

			ListView_SetItemText(listView, finalIndex, 0, value.data());
			ListView_SetItemText(listView, finalIndex, 1, option.name.data());
		}
	}
}

bool OptionsDialog::GetBooleanConfigValue(OptionsDialog::AdvancedOptionId id)
{
	switch (id)
	{
	case AdvancedOptionId::CheckSystemIsPinnedToNameSpaceTree:
		return m_config->checkPinnedToNamespaceTreeProperty;

	case AdvancedOptionId::OpenTabsInForeground:
		return m_config->openTabsInForeground;

	case AdvancedOptionId::GoUpOnDoubleClick:
		return m_config->goUpOnDoubleClick;

	default:
		assert(false);
		break;
	}

	return false;
}

void OptionsDialog::SetBooleanConfigValue(OptionsDialog::AdvancedOptionId id, bool value)
{
	switch (id)
	{
	case OptionsDialog::AdvancedOptionId::CheckSystemIsPinnedToNameSpaceTree:
		m_config->checkPinnedToNamespaceTreeProperty = value;
		break;

	case AdvancedOptionId::OpenTabsInForeground:
		m_config->openTabsInForeground = value;
		break;

	case AdvancedOptionId::GoUpOnDoubleClick:
		m_config->goUpOnDoubleClick = value;
		break;

	default:
		assert(false);
		break;
	}
}

OptionsDialog::AdvancedOption *OptionsDialog::GetAdvancedOptionByIndex(HWND dlg, int index)
{
	LVITEM lvItem;
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = index;
	lvItem.iSubItem = 0;
	BOOL res = ListView_GetItem(GetDlgItem(dlg, IDC_ADVANCED_OPTIONS), &lvItem);

	if (!res)
	{
		throw std::runtime_error("Item lookup failed");
	}

	return reinterpret_cast<AdvancedOption *>(lvItem.lParam);
}

INT_PTR OptionsDialog::OnPageCtlColorDlg(HWND hwnd, HDC hdc)
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
	HRESULT hr;

	/* Load the dialog helper message. */
	std::wstring helperText =
		ResourceHelper::LoadString(m_resourceInstance, IDS_DEFAULTSETTINGS_NEWTAB);

	GetDlgItemText(hDlg, IDC_DEFAULT_NEWTABDIR_EDIT, szNewTabDir, SIZEOF_ARRAY(szNewTabDir));

	std::wstring virtualParsingPath;
	hr = DecodeFriendlyPath(szNewTabDir, virtualParsingPath);

	if (SUCCEEDED(hr))
	{
		StringCchCopy(g_szNewTabDirectory, SIZEOF_ARRAY(g_szNewTabDirectory),
			virtualParsingPath.c_str());
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

template <typename T>
void OptionsDialog::AddItemsToComboBox(HWND comboBox, const std::vector<T> &itemIds,
	T currentItemId, std::function<UINT(T)> getStringResourceId)
{
	for (auto itemId : itemIds)
	{
		UINT stringResourceId = getStringResourceId(itemId);
		std::wstring itemName = ResourceHelper::LoadString(m_resourceInstance, stringResourceId);

		int index = static_cast<int>(
			SendMessage(comboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(itemName.c_str())));

		if (index == CB_ERR)
		{
			assert(false);
			continue;
		}

		[[maybe_unused]] auto res =
			SendMessage(comboBox, CB_SETITEMDATA, index, static_cast<LPARAM>(itemId));
		assert(res != CB_ERR);

		if (itemId == currentItemId)
		{
			res = SendMessage(comboBox, CB_SETCURSEL, index, 0);
			assert(res != CB_ERR);
		}
	}
}

UINT GetSizeDisplayFormatStringResourceId(SizeDisplayFormat sizeDisplayFormat)
{
	switch (sizeDisplayFormat)
	{
		break;
	case SizeDisplayFormat::Bytes:
		return IDS_OPTIONS_DIALOG_FILE_SIZE_BYTES;

	case SizeDisplayFormat::KB:
		return IDS_OPTIONS_DIALOG_FILE_SIZE_KB;

	case SizeDisplayFormat::MB:
		return IDS_OPTIONS_DIALOG_FILE_SIZE_MB;

	case SizeDisplayFormat::GB:
		return IDS_OPTIONS_DIALOG_FILE_SIZE_GB;

	case SizeDisplayFormat::TB:
		return IDS_OPTIONS_DIALOG_FILE_SIZE_TB;

	case SizeDisplayFormat::PB:
		return IDS_OPTIONS_DIALOG_FILE_SIZE_PB;

	// SizeDisplayFormat::None isn't an option that's displayed to the user, so there should never
	// be a string lookup for that item.
	case SizeDisplayFormat::None:
	default:
		throw std::runtime_error("SizeDisplayFormat value not found or invalid");
	}
}

UINT GetIconSetStringResourceId(IconSet iconSet)
{
	switch (iconSet)
	{
	case IconSet::Color:
		return IDS_ICON_SET_COLOR;

	case IconSet::FluentUi:
		return IDS_ICON_SET_FLUENT_UI;

	case IconSet::Windows10:
		return IDS_ICON_SET_WINDOWS_10;

	default:
		throw std::runtime_error("IconSet value not found");
	}
}

UINT GetThemeStringResourceId(Theme theme)
{
	switch (theme)
	{
	case Theme::Light:
		return IDS_THEME_LIGHT;

	case Theme::Dark:
		return IDS_THEME_DARK;

	case Theme::System:
		return IDS_THEME_SYSTEM_DEFAULT;

	default:
		throw std::runtime_error("Theme value not found");
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

BOOL OptionsDialog::AddLanguageToComboBox(HWND hComboBox, const TCHAR *szImageDirectory,
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
