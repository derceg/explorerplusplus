// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserCommandController.h"
#include "AboutDialog.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "DisplayColoursDialog.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "SortModeMenuMappings.h"
#include "SystemFontHelper.h"
#include "TabContainer.h"
#include "UpdateCheckDialog.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/DpiCompatibility.h"

using namespace std::string_literals;

namespace
{

constexpr bool AreSelectTabItemIdsContiguous()
{
	constexpr int ids[] = { IDA_SELECT_TAB_1, IDA_SELECT_TAB_2, IDA_SELECT_TAB_3, IDA_SELECT_TAB_4,
		IDA_SELECT_TAB_5, IDA_SELECT_TAB_6, IDA_SELECT_TAB_7, IDA_SELECT_TAB_8 };

	for (size_t i = 1; i < std::size(ids); i++)
	{
		if (ids[i] != (ids[i - 1] + 1))
		{
			return false;
		}
	}

	return true;
}

// It's assumed below that the select tab item IDs are contiguous, which is why this check is
// present.
static_assert(AreSelectTabItemIdsContiguous());

}

BrowserCommandController::BrowserCommandController(BrowserWindow *browser, Config *config,
	ClipboardStore *clipboardStore, const ResourceLoader *resourceLoader) :
	m_browser(browser),
	m_config(config),
	m_clipboardStore(clipboardStore),
	m_resourceLoader(resourceLoader)
{
}

bool BrowserCommandController::IsCommandEnabled(int command) const
{
	if (IsCommandContextSensitive(command))
	{
		return m_browser->GetCommandTargetManager()->GetCurrentTarget()->IsCommandEnabled(command);
	}

	switch (command)
	{
	case IDM_FILE_SAVEDIRECTORYLISTING:
		return GetActiveShellBrowser()->CanSaveDirectoryListing();

	case IDM_FILE_OPENCOMMANDPROMPT:
	case IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR:
		return CanStartCommandPrompt();

	case IDM_EDIT_WILDCARDDESELECT:
		return GetActiveShellBrowser()->CanStartWildcardSelection(SelectionType::Deselect);

	case IDM_VIEW_AUTOSIZECOLUMNS:
		return GetActiveShellBrowser()->CanAutoSizeColumns();

	case IDM_ACTIONS_NEWFOLDER:
		return GetActiveShellBrowser()->CanCreateNewFolder();

	case IDM_ACTIONS_SPLITFILE:
		return GetActiveShellBrowser()->CanSplitFile();

	case IDM_ACTIONS_MERGEFILES:
		return GetActiveShellBrowser()->CanMergeFiles();

	case IDM_GO_BACK:
		return GetActiveShellBrowser()->GetNavigationController()->CanGoBack();

	case IDM_GO_FORWARD:
		return GetActiveShellBrowser()->GetNavigationController()->CanGoForward();

	case IDM_GO_UP:
		return GetActiveShellBrowser()->GetNavigationController()->CanGoUp();

	default:
		DCHECK(false);
		return false;
	}
}

bool BrowserCommandController::CanStartCommandPrompt() const
{
	const auto *shellBrowser = GetActiveShellBrowser();

	SFGAOF attributes = SFGAO_FILESYSTEM | SFGAO_STREAM;
	HRESULT hr = GetItemAttributes(shellBrowser->GetDirectory().Raw(), &attributes);

	if (FAILED(hr))
	{
		return false;
	}

	// SFGAO_STREAM will be set if the item is a container file (e.g. a .zip file). It's not
	// possible to show a command prompt in that case.
	//
	// Note that if the item is a nested folder within a container file, the SFGAO_FILESYSTEM
	// attribute won't be set, so checking SFGAO_STREAM is only necessary for the top-level folder
	// within a container file.
	if (WI_IsFlagClear(attributes, SFGAO_FILESYSTEM) || WI_IsFlagSet(attributes, SFGAO_STREAM))
	{
		return false;
	}

	return true;
}

void BrowserCommandController::ExecuteCommand(int command, OpenFolderDisposition disposition)
{
	if (IsCommandContextSensitive(command))
	{
		m_browser->GetCommandTargetManager()->GetCurrentTarget()->ExecuteCommand(command);
		return;
	}
	else if (IsSortModeMenuItemId(command))
	{
		OnSortBy(GetSortModeForMenuItemId(command));
		return;
	}

	switch (command)
	{
	case IDM_FILE_CLOSETAB:
		OnCloseTab();
		break;

	case IDM_FILE_SAVEDIRECTORYLISTING:
		GetActiveShellBrowser()->SaveDirectoryListing();
		break;

	case IDM_FILE_OPENCOMMANDPROMPT:
		StartCommandPrompt();
		break;

	case IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR:
		StartCommandPrompt(LaunchProcessFlags::Elevated);
		break;

	case IDM_FILE_COPYFOLDERPATH:
		CopyFolderPath();
		break;

	case IDM_EDIT_SELECTALL:
		GetActiveShellBrowser()->SelectAllItems();
		break;

	case IDM_EDIT_INVERTSELECTION:
		GetActiveShellBrowser()->InvertSelection();
		break;

	case IDM_EDIT_SELECTNONE:
		GetActiveShellBrowser()->ClearSelection();
		break;

	case IDM_EDIT_WILDCARDSELECTION:
		GetActiveShellBrowser()->StartWildcardSelection(SelectionType::Select);
		break;

	case IDM_EDIT_WILDCARDDESELECT:
		GetActiveShellBrowser()->StartWildcardSelection(SelectionType::Deselect);
		break;

	case IDM_VIEW_STATUSBAR:
		m_config->showStatusBar = !m_config->showStatusBar.get();
		break;

	case IDM_VIEW_FOLDERS:
		m_config->showFolders = !m_config->showFolders.get();
		break;

	case IDM_VIEW_DISPLAYWINDOW:
		m_config->showDisplayWindow = !m_config->showDisplayWindow.get();
		break;

	case IDM_VIEW_TOOLBARS_ADDRESS_BAR:
		m_config->showAddressBar = !m_config->showAddressBar.get();
		break;

	case IDM_VIEW_TOOLBARS_MAIN_TOOLBAR:
		m_config->showMainToolbar = !m_config->showMainToolbar.get();
		break;

	case IDM_VIEW_TOOLBARS_BOOKMARKS_TOOLBAR:
		m_config->showBookmarksToolbar = !m_config->showBookmarksToolbar.get();
		break;

	case IDM_VIEW_TOOLBARS_DRIVES_TOOLBAR:
		m_config->showDrivesToolbar = !m_config->showDrivesToolbar.get();
		break;

	case IDM_VIEW_TOOLBARS_APPLICATION_TOOLBAR:
		m_config->showApplicationToolbar = !m_config->showApplicationToolbar.get();
		break;

	case IDM_VIEW_TOOLBARS_LOCK_TOOLBARS:
		m_config->lockToolbars = !m_config->lockToolbars.get();
		break;

	case IDM_VIEW_TOOLBARS_CUSTOMIZE:
		m_browser->StartMainToolbarCustomization();
		break;

	case IDM_VIEW_DECREASE_TEXT_SIZE:
		OnChangeMainFontSize(FontSizeType::Decrease);
		break;

	case IDM_VIEW_INCREASE_TEXT_SIZE:
		OnChangeMainFontSize(FontSizeType::Increase);
		break;

	case IDA_RESET_TEXT_SIZE:
		OnResetMainFontSize();
		break;

	case IDM_VIEW_EXTRALARGEICONS:
		GetActiveShellBrowser()->SetViewMode(ViewMode::ExtraLargeIcons);
		break;

	case IDM_VIEW_LARGEICONS:
		GetActiveShellBrowser()->SetViewMode(ViewMode::LargeIcons);
		break;

	case IDM_VIEW_ICONS:
		GetActiveShellBrowser()->SetViewMode(ViewMode::Icons);
		break;

	case IDM_VIEW_SMALLICONS:
		GetActiveShellBrowser()->SetViewMode(ViewMode::SmallIcons);
		break;

	case IDM_VIEW_LIST:
		GetActiveShellBrowser()->SetViewMode(ViewMode::List);
		break;

	case IDM_VIEW_DETAILS:
		GetActiveShellBrowser()->SetViewMode(ViewMode::Details);
		break;

	case IDM_VIEW_EXTRALARGETHUMBNAILS:
		GetActiveShellBrowser()->SetViewMode(ViewMode::ExtraLargeThumbnails);
		break;

	case IDM_VIEW_LARGETHUMBNAILS:
		GetActiveShellBrowser()->SetViewMode(ViewMode::LargeThumbnails);
		break;

	case IDM_VIEW_THUMBNAILS:
		GetActiveShellBrowser()->SetViewMode(ViewMode::Thumbnails);
		break;

	case IDM_VIEW_TILES:
		GetActiveShellBrowser()->SetViewMode(ViewMode::Tiles);
		break;

	case IDM_VIEW_AUTOARRANGE:
		GetActiveShellBrowser()->SetAutoArrangeEnabled(
			!GetActiveShellBrowser()->IsAutoArrangeEnabled());
		break;

	case IDM_SORT_ASCENDING:
		GetActiveShellBrowser()->SetSortDirection(SortDirection::Ascending);
		break;

	case IDM_SORT_DESCENDING:
		GetActiveShellBrowser()->SetSortDirection(SortDirection::Descending);
		break;

	case IDM_VIEW_REFRESH:
		GetActiveShellBrowser()->GetNavigationController()->Refresh();
		break;

	case IDM_VIEW_AUTOSIZECOLUMNS:
		GetActiveShellBrowser()->AutoSizeColumns();
		break;

	case IDM_VIEW_CHANGEDISPLAYCOLOURS:
		OnChangeDisplayColors();
		break;

	case IDM_FILTER_FILTERRESULTS:
		GetActiveShellBrowser()->EditFilterSettings();
		break;

	case IDM_FILTER_ENABLE_FILTER:
		GetActiveShellBrowser()->SetFilterEnabled(!GetActiveShellBrowser()->IsFilterEnabled());
		break;

	case IDM_ACTIONS_NEWFOLDER:
		GetActiveShellBrowser()->CreateNewFolder();
		break;

	case IDM_ACTIONS_SPLITFILE:
		GetActiveShellBrowser()->SplitFile();
		break;

	case IDM_ACTIONS_MERGEFILES:
		GetActiveShellBrowser()->MergeFiles();
		break;

	case IDM_GO_BACK:
		GoBack(disposition);
		break;

	case IDM_GO_FORWARD:
		GoForward(disposition);
		break;

	case IDM_GO_UP:
		GoUp(disposition);
		break;

	case IDM_GO_QUICK_ACCESS:
		GoToPath(QUICK_ACCESS_PATH, disposition);
		break;

	case IDM_GO_COMPUTER:
		GoToKnownFolder(FOLDERID_ComputerFolder, disposition);
		break;

	case IDM_GO_DOCUMENTS:
		GoToKnownFolder(FOLDERID_Documents, disposition);
		break;

	case IDM_GO_DOWNLOADS:
		GoToKnownFolder(FOLDERID_Downloads, disposition);
		break;

	case IDM_GO_MUSIC:
		GoToKnownFolder(FOLDERID_Music, disposition);
		break;

	case IDM_GO_PICTURES:
		GoToKnownFolder(FOLDERID_Pictures, disposition);
		break;

	case IDM_GO_VIDEOS:
		GoToKnownFolder(FOLDERID_Videos, disposition);
		break;

	case IDM_GO_DESKTOP:
		GoToKnownFolder(FOLDERID_Desktop, disposition);
		break;

	case IDM_GO_RECYCLE_BIN:
		GoToKnownFolder(FOLDERID_RecycleBinFolder, disposition);
		break;

	case IDM_GO_CONTROL_PANEL:
		GoToKnownFolder(FOLDERID_ControlPanelFolder, disposition);
		break;

	case IDM_GO_PRINTERS:
		GoToKnownFolder(FOLDERID_PrintersFolder, disposition);
		break;

	case IDM_GO_NETWORK:
		GoToKnownFolder(FOLDERID_NetworkFolder, disposition);
		break;

	case IDM_GO_WSL_DISTRIBUTIONS:
		GoToPath(WSL_DISTRIBUTIONS_PATH, disposition);
		break;

	case IDM_HELP_ONLINE_DOCUMENTATION:
		OnOpenOnlineDocumentation();
		break;

	case IDM_HELP_CHECKFORUPDATES:
		OnCheckForUpdates();
		break;

	case IDM_HELP_ABOUT:
		OnAbout();
		break;

	case IDA_SELECT_PREVIOUS_TAB:
		m_browser->GetActiveTabContainer()->SelectAdjacentTab(
			TabContainer::SelectionDirection::Previous);
		break;

	case IDA_SELECT_NEXT_TAB:
		m_browser->GetActiveTabContainer()->SelectAdjacentTab(
			TabContainer::SelectionDirection::Next);
		break;

	case IDA_DUPLICATE_TAB:
		OnDuplicateTab();
		break;

	case IDA_HOME:
		GetActiveShellBrowser()->GetNavigationController()->Navigate(m_config->defaultTabDirectory);
		break;

	case IDA_SELECT_TAB_1:
	case IDA_SELECT_TAB_2:
	case IDA_SELECT_TAB_3:
	case IDA_SELECT_TAB_4:
	case IDA_SELECT_TAB_5:
	case IDA_SELECT_TAB_6:
	case IDA_SELECT_TAB_7:
	case IDA_SELECT_TAB_8:
		OnSelectTabAtIndex(command - IDA_SELECT_TAB_1);
		break;

	case IDA_SELECT_LAST_TAB:
		OnSelectLastTab();
		break;

	default:
		DCHECK(false);
		break;
	}
}

bool BrowserCommandController::IsCommandContextSensitive(int command) const
{
	switch (command)
	{
	// These commands are context-sensitive (i.e. they depend on the active target).
	case IDM_FILE_COPYITEMPATH:
	case IDM_FILE_DELETE:
	case IDM_FILE_DELETEPERMANENTLY:
	case IDM_FILE_RENAME:
	case IDM_FILE_PROPERTIES:
	case IDM_EDIT_CUT:
	case IDM_EDIT_COPY:
	case IDM_EDIT_MOVETOFOLDER:
	case IDM_EDIT_COPYTOFOLDER:
		return true;

	default:
		return false;
	}
}

void BrowserCommandController::OnSortBy(SortMode sortMode)
{
	auto *shellBrowser = GetActiveShellBrowser();
	SortMode currentSortMode = shellBrowser->GetSortMode();

	if (sortMode == currentSortMode)
	{
		shellBrowser->SetSortDirection(InvertSortDirection(shellBrowser->GetSortDirection()));
	}
	else
	{
		shellBrowser->SetSortMode(sortMode);
	}
}

void BrowserCommandController::OnCloseTab()
{
	auto *tabContainer = m_browser->GetActiveTabContainer();
	tabContainer->CloseTab(tabContainer->GetSelectedTab());
}

void BrowserCommandController::StartCommandPrompt(LaunchProcessFlags flags)
{
	wil::unique_cotaskmem_string systemPath;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_System, KF_FLAG_DEFAULT, nullptr, &systemPath);

	if (FAILED(hr))
	{
		return;
	}

	std::filesystem::path fullPath(systemPath.get());
	fullPath /= L"cmd.exe";

	const auto *shellBrowser = GetActiveShellBrowser();

	wil::unique_cotaskmem_string directoryPath;
	hr = SHGetNameFromIDList(shellBrowser->GetDirectory().Raw(), SIGDN_FILESYSPATH, &directoryPath);

	if (FAILED(hr))
	{
		return;
	}

	std::wstring parameters;

	if (WI_IsFlagSet(flags, LaunchProcessFlags::Elevated))
	{
		parameters = L"/K cd /d "s + directoryPath.get();
	}

	LaunchProcess(nullptr, fullPath.c_str(), parameters, directoryPath.get(), flags);
}

void BrowserCommandController::CopyFolderPath() const
{
	const auto *shellBrowser = GetActiveShellBrowser();

	wil::unique_cotaskmem_string path;
	HRESULT hr = SHGetNameFromIDList(shellBrowser->GetDirectory().Raw(),
		SIGDN_DESKTOPABSOLUTEPARSING, &path);

	if (FAILED(hr))
	{
		return;
	}

	BulkClipboardWriter clipboardWriter(m_clipboardStore);
	clipboardWriter.WriteText(path.get());
}

void BrowserCommandController::OnChangeMainFontSize(FontSizeType sizeType)
{
	auto &mainFont = m_config->mainFont.get();
	std::wstring updatedFontName;
	int updatedFontSize;

	if (mainFont)
	{
		updatedFontName = mainFont->GetName();
		updatedFontSize = mainFont->GetSize();
	}
	else
	{
		auto systemLogFont = GetDefaultSystemFontForDefaultDpi();
		int systemFontSize = std::abs(
			DpiCompatibility::GetInstance().PixelsToPointsForDefaultDpi(systemLogFont.lfHeight));

		updatedFontName = systemLogFont.lfFaceName;
		updatedFontSize = systemFontSize;
	}

	if (sizeType == FontSizeType::Decrease)
	{
		updatedFontSize -= FONT_SIZE_CHANGE_DELTA;
	}
	else
	{
		updatedFontSize += FONT_SIZE_CHANGE_DELTA;
	}

	m_config->mainFont = CustomFont(updatedFontName, updatedFontSize);
}

void BrowserCommandController::OnResetMainFontSize()
{
	auto &mainFont = m_config->mainFont.get();

	if (!mainFont)
	{
		// The default font is being used, so the font size is currently the default size and
		// nothing needs to change.
		return;
	}

	auto systemLogFont = GetDefaultSystemFontForDefaultDpi();
	int systemFontSize = std::abs(
		DpiCompatibility::GetInstance().PixelsToPointsForDefaultDpi(systemLogFont.lfHeight));

	// Different fonts can have different metrics, so there isn't really a concept of a "default"
	// font size. The size of the default system font is taken as a reasonable proxy. This also
	// means that if the user only changes the font size (while still using the system font),
	// resetting the font size will work as expected.
	m_config->mainFont = CustomFont(mainFont->GetName(), systemFontSize);
}

void BrowserCommandController::OnChangeDisplayColors()
{
	DisplayColoursDialog displayColoursDialog(m_resourceLoader, m_browser->GetHWND(), m_config);
	displayColoursDialog.ShowModalDialog();
}

void BrowserCommandController::GoBack(OpenFolderDisposition disposition)
{
	auto *shellBrowser = GetActiveShellBrowser();

	if (disposition == OpenFolderDisposition::CurrentTab)
	{
		shellBrowser->GetNavigationController()->GoBack();
	}
	else
	{
		auto *entry = shellBrowser->GetNavigationController()->GetEntry(-1);

		if (!entry)
		{
			return;
		}

		m_browser->OpenItem(entry->GetPidl().Raw(), disposition);
	}
}

void BrowserCommandController::GoForward(OpenFolderDisposition disposition)
{
	auto *shellBrowser = GetActiveShellBrowser();

	if (disposition == OpenFolderDisposition::CurrentTab)
	{
		shellBrowser->GetNavigationController()->GoForward();
	}
	else
	{
		auto *entry = shellBrowser->GetNavigationController()->GetEntry(1);

		if (!entry)
		{
			return;
		}

		m_browser->OpenItem(entry->GetPidl().Raw(), disposition);
	}
}

void BrowserCommandController::GoUp(OpenFolderDisposition disposition)
{
	auto *shellBrowser = GetActiveShellBrowser();

	if (disposition == OpenFolderDisposition::CurrentTab)
	{
		shellBrowser->GetNavigationController()->GoUp();
	}
	else
	{
		auto *currentEntry = shellBrowser->GetNavigationController()->GetCurrentEntry();

		unique_pidl_absolute pidlParent;
		HRESULT hr =
			GetVirtualParentPath(currentEntry->GetPidl().Raw(), wil::out_param(pidlParent));

		if (FAILED(hr))
		{
			return;
		}

		m_browser->OpenItem(pidlParent.get(), disposition);
	}
}

void BrowserCommandController::GoToPath(const std::wstring &path, OpenFolderDisposition disposition)
{
	m_browser->OpenItem(path, disposition);
}

void BrowserCommandController::GoToKnownFolder(REFKNOWNFOLDERID knownFolderId,
	OpenFolderDisposition disposition)
{
	unique_pidl_absolute pidl;
	HRESULT hr =
		SHGetKnownFolderIDList(knownFolderId, KF_FLAG_DEFAULT, nullptr, wil::out_param(pidl));

	if (FAILED(hr))
	{
		return;
	}

	m_browser->OpenItem(pidl.get(), disposition);
}

void BrowserCommandController::OnOpenOnlineDocumentation()
{
	ShellExecute(nullptr, L"open", DOCUMENTATION_URL, nullptr, nullptr, SW_SHOWNORMAL);
}

void BrowserCommandController::OnCheckForUpdates()
{
	UpdateCheckDialog updateCheckDialog(m_resourceLoader, m_browser->GetHWND());
	updateCheckDialog.ShowModalDialog();
}

void BrowserCommandController::OnAbout()
{
	AboutDialog aboutDialog(m_resourceLoader, m_browser->GetHWND());
	aboutDialog.ShowModalDialog();
}

void BrowserCommandController::OnDuplicateTab()
{
	auto *tabContainer = m_browser->GetActiveTabContainer();
	tabContainer->DuplicateTab(tabContainer->GetSelectedTab());
}

void BrowserCommandController::OnSelectTabAtIndex(int index)
{
	auto *tabContainer = m_browser->GetActiveTabContainer();
	int numTabs = tabContainer->GetNumTabs();
	int selectionIndex;

	if (index < numTabs)
	{
		selectionIndex = index;
	}
	else
	{
		selectionIndex = numTabs - 1;
	}

	tabContainer->SelectTabAtIndex(selectionIndex);
}

void BrowserCommandController::OnSelectLastTab()
{
	auto *tabContainer = m_browser->GetActiveTabContainer();
	int numTabs = tabContainer->GetNumTabs();
	tabContainer->SelectTabAtIndex(numTabs - 1);
}

ShellBrowser *BrowserCommandController::GetActiveShellBrowser()
{
	return m_browser->GetActiveShellBrowser();
}

const ShellBrowser *BrowserCommandController::GetActiveShellBrowser() const
{
	return m_browser->GetActiveShellBrowser();
}
