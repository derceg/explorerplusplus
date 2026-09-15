// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AcceleratorHelper.h"
#include "AppServices.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Config.h"
#include "FeatureList.h"
#include "FrequentLocationsMenu.h"
#include "HistoryMenu.h"
#include "Icon.h"
#include "MainMenuSubMenuHost.h"
#include "MainResource.h"
#include "MenuRanges.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "SortMenuBuilder.h"
#include "TabContainer.h"
#include "TabRestorerMenu.h"
#include "ViewsMenuBuilder.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"
#include <wil/resource.h>
#include <map>

const std::map<UINT, Icon> MAIN_MENU_IMAGE_MAPPINGS = {
	{ IDM_FILE_NEWTAB, Icon::NewTab },
	{ IDM_FILE_CLOSETAB, Icon::CloseTab },
	{ IDM_FILE_OPENCOMMANDPROMPT, Icon::CommandLine },
	{ IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR, Icon::CommandLineAdmin },
	{ IDM_FILE_DELETE, Icon::Delete },
	{ IDM_FILE_DELETEPERMANENTLY, Icon::DeletePermanently },
	{ IDM_FILE_RENAME, Icon::Rename },
	{ IDM_FILE_PROPERTIES, Icon::Properties },

	{ IDM_EDIT_UNDO, Icon::Undo },
	{ IDM_EDIT_COPY, Icon::Copy },
	{ IDM_EDIT_CUT, Icon::Cut },
	{ IDM_EDIT_PASTE, Icon::Paste },
	{ IDM_EDIT_PASTESHORTCUT, Icon::PasteShortcut },

	{ IDM_EDIT_COPYTOFOLDER, Icon::CopyTo },
	{ IDM_EDIT_MOVETOFOLDER, Icon::MoveTo },

	{ IDM_ACTIONS_NEWFOLDER, Icon::NewFolder },
	{ IDM_ACTIONS_SPLITFILE, Icon::SplitFiles },
	{ IDM_ACTIONS_MERGEFILES, Icon::MergeFiles },

	{ IDM_VIEW_REFRESH, Icon::Refresh },
	{ IDM_VIEW_SELECTCOLUMNS, Icon::SelectColumns },

	{ IDM_FILTER_FILTERRESULTS, Icon::Filter },

	{ IDM_GO_BACK, Icon::Back },
	{ IDM_GO_FORWARD, Icon::Forward },
	{ IDM_GO_UP, Icon::Up },

	{ IDM_TOOLS_SEARCH, Icon::Search },
	{ IDM_TOOLS_CUSTOMIZECOLORS, Icon::CustomizeColors },
	{ IDM_TOOLS_OPTIONS, Icon::Options },

	{ IDM_HELP_ONLINE_DOCUMENTATION, Icon::Help },
};

void Explorerplusplus::InitializeMainMenu()
{
	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_mainMenuSystemImageList)));

	// These need to occur after the language module has been initialized, but
	// before the tabs are restored.
	HMENU mainMenu = LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_MAINMENU));

	if (!m_featureList->IsEnabled(Feature::MultipleWindowsPerSession))
	{
		DeleteMenu(mainMenu, IDM_FILE_NEW_WINDOW, MF_BYCOMMAND);
	}
	else
	{
		// TODO: Selecting clone window launches a separate process. That doesn't fit in with how
		// MultipleWindowsPerSession is designed to work and the menu item should be removed when
		// the feature is enabled by default.
		DeleteMenu(mainMenu, IDM_FILE_CLONEWINDOW, MF_BYCOMMAND);
	}

	if (!m_featureList->IsEnabled(Feature::DualPane))
	{
		DeleteMenu(mainMenu, IDM_VIEW_DUAL_PANE, MF_BYCOMMAND);
	}

	if (!m_featureList->IsEnabled(Feature::Plugins))
	{
		DeleteMenu(mainMenu, IDM_TOOLS_RUNSCRIPT, MF_BYCOMMAND);
	}

	SetMenu(m_hwnd, mainMenu);

	AddMainMenuSubmenu(mainMenu, IDM_FILE_REOPEN_RECENT_TAB,
		[this](MenuView *menuView)
		{
			return std::make_unique<TabRestorerMenu>(menuView, m_acceleratorManager,
				m_appServices->GetTabRestorer(), &m_shellIconLoader, m_resourceLoader,
				MENU_RECENT_TABS_START_ID, MENU_RECENT_TABS_END_ID);
		});

	ViewsMenuBuilder viewsMenuBuilder(m_resourceLoader);
	viewsMenuBuilder.AddViewModesToMenu(mainMenu, IDM_VIEW_PLACEHOLDER, false);
	DeleteMenu(mainMenu, IDM_VIEW_PLACEHOLDER, MF_BYCOMMAND);

	SetMainMenuImages();

	InitializeGoMenu(mainMenu);

	AddMainMenuSubmenu(mainMenu, IDM_GO_HISTORY,
		[this](MenuView *menuView)
		{
			return std::make_unique<HistoryMenu>(menuView, m_acceleratorManager,
				m_appServices->GetHistoryModel(), this, &m_shellIconLoader, MENU_HISTORY_START_ID,
				MENU_HISTORY_END_ID);
		});

	AddMainMenuSubmenu(mainMenu, IDM_GO_FREQUENT_LOCATIONS,
		[this](MenuView *menuView)
		{
			return std::make_unique<FrequentLocationsMenu>(menuView, m_acceleratorManager,
				m_appServices->GetFrequentLocationsModel(), this, &m_shellIconLoader,
				MENU_FREQUENT_LOCATIONS_START_ID, MENU_FREQUENT_LOCATIONS_END_ID);
		});

	UpdateMenuAcceleratorStrings(mainMenu, m_acceleratorManager);
}

void Explorerplusplus::AddMainMenuSubmenu(HMENU mainMenu, UINT subMenuItemId,
	std::function<std::unique_ptr<MenuBase>(MenuView *menuView)> menuCreator)
{
	auto menuHost = std::make_unique<MainMenuSubMenuHost>(this, mainMenu, subMenuItemId);
	auto menu = menuCreator(menuHost->GetView());
	m_mainMenuSubMenus.emplace_back(std::move(menuHost), std::move(menu));
}

void Explorerplusplus::SetMainMenuImages()
{
	HMENU mainMenu = GetMenu(m_hwnd);
	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_hwnd);

	for (const auto &mapping : MAIN_MENU_IMAGE_MAPPINGS)
	{
		ResourceHelper::SetMenuItemImage(mainMenu, mapping.first, m_resourceLoader, mapping.second,
			dpi, m_mainMenuImages);
	}

	SetPasteSymLinkElevationIcon();
}

void Explorerplusplus::SetPasteSymLinkElevationIcon()
{
	// Creating a symlink typically requires elevation. However, if the application is already
	// elevated, there's no need to show the shield icon (which is used to indicate that elevation
	// is required).
	// Note that elevation isn't required if developer mode is enabled on Windows 10 and above.
	// Since the status of developer mode isn't checked here, the shield icon may be shown in cases
	// where elevation isn't actually required. That's not too much of an issue, since the icon here
	// is considered to be a hint that elevation may be required.
	if (IsProcessElevated())
	{
		return;
	}

	SHSTOCKICONINFO info = {};
	info.cbSize = sizeof(info);
	HRESULT hr = SHGetStockIconInfo(SIID_SHIELD, SHGSI_SYSICONINDEX, &info);

	if (FAILED(hr))
	{
		DCHECK(false);
		return;
	}

	wil::unique_hbitmap bitmap;
	ImageHelper::CreateHBITMAPFromImageListIcon(m_mainMenuSystemImageList.get(),
		info.iSysImageIndex, bitmap);

	HMENU mainMenu = GetMenu(m_hwnd);
	MenuHelper::SetBitmapForItem(mainMenu, IDM_EDIT_PASTE_SYMBOLIC_LINK, bitmap.get());
	m_mainMenuImages.push_back(std::move(bitmap));
}

void Explorerplusplus::InitializeGoMenu(HMENU mainMenu)
{
	// This is a bit indirect, but it's better than using something like GetSubMenu(), which would
	// rely on the "Go" menu remaining in a fixed position.
	HMENU goMenu = MenuHelper::FindParentMenu(mainMenu, IDM_GO_BACK);
	CHECK(goMenu);

	MenuHelper::AddSeparator(goMenu);

	// This is the quick access/home folder in Windows 10/11.
	AddGoMenuItem(goMenu, IDM_GO_QUICK_ACCESS, QUICK_ACCESS_PATH);
	AddGoMenuItem(goMenu, IDM_GO_COMPUTER, FOLDERID_ComputerFolder);

	MenuHelper::AddSeparator(goMenu);

	AddGoMenuItem(goMenu, IDM_GO_DOCUMENTS, FOLDERID_Documents);
	AddGoMenuItem(goMenu, IDM_GO_DOWNLOADS, FOLDERID_Downloads);
	AddGoMenuItem(goMenu, IDM_GO_MUSIC, FOLDERID_Music);
	AddGoMenuItem(goMenu, IDM_GO_PICTURES, FOLDERID_Pictures);
	AddGoMenuItem(goMenu, IDM_GO_VIDEOS, FOLDERID_Videos);
	AddGoMenuItem(goMenu, IDM_GO_DESKTOP, FOLDERID_Desktop);

	MenuHelper::AddSeparator(goMenu);

	AddGoMenuItem(goMenu, IDM_GO_RECYCLE_BIN, FOLDERID_RecycleBinFolder);
	AddGoMenuItem(goMenu, IDM_GO_CONTROL_PANEL, FOLDERID_ControlPanelFolder);
	AddGoMenuItem(goMenu, IDM_GO_PRINTERS, FOLDERID_PrintersFolder);
	AddGoMenuItem(goMenu, IDM_GO_NETWORK, FOLDERID_NetworkFolder);

	MenuHelper::AddSeparator(goMenu);

	AddGoMenuItem(goMenu, IDM_GO_WSL_DISTRIBUTIONS, WSL_DISTRIBUTIONS_PATH);

	MenuHelper::RemoveDuplicateSeperators(goMenu);
	MenuHelper::RemoveTrailingSeparators(goMenu);
}

void Explorerplusplus::AddGoMenuItem(HMENU goMenu, UINT id, const KNOWNFOLDERID &folderId)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHGetKnownFolderIDList(folderId, KF_FLAG_DEFAULT, nullptr, wil::out_param(pidl));

	if (FAILED(hr))
	{
		return;
	}

	AddGoMenuItem(goMenu, id, pidl.get());
}

void Explorerplusplus::AddGoMenuItem(HMENU goMenu, UINT id, const std::wstring &path)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(path.c_str(), nullptr, wil::out_param(pidl), 0, nullptr);

	if (FAILED(hr))
	{
		return;
	}

	AddGoMenuItem(goMenu, id, pidl.get());
}

void Explorerplusplus::AddGoMenuItem(HMENU goMenu, UINT id, PCIDLIST_ABSOLUTE pidl)
{
	std::wstring folderName;
	HRESULT hr = GetDisplayName(pidl, SHGDN_INFOLDER, folderName);

	if (FAILED(hr))
	{
		return;
	}

	MenuHelper::AddStringItem(goMenu, id, folderName);

	m_iconFetcher.QueueIconTask(pidl,
		[this, goMenu, id](int iconIndex, int overlayIndex)
		{
			UNREFERENCED_PARAMETER(overlayIndex);

			// Accessing the Explorerplusplus instance here should always be safe. This callback is
			// run on the main thread and will either run before the instance is destroyed, or not
			// at all. It's not feasible for the callback to run while the destruction of the
			// Explorerplusplus instance is ongoing (which would be unsafe), since even if messages
			// were pumped, the window message handler that the class sets up will no longer be
			// active. So, once destruction of the Explorerplusplus instance has started, there's no
			// way for this callback to run.
			wil::unique_hbitmap bitmap;
			ImageHelper::CreateHBITMAPFromImageListIcon(m_mainMenuSystemImageList.get(), iconIndex,
				bitmap);

			MenuHelper::SetBitmapForItem(goMenu, id, bitmap.get());

			m_mainMenuImages.push_back(std::move(bitmap));
		});
}

boost::signals2::connection Explorerplusplus::AddMainMenuPreShowObserver(
	const MainMenuPreShowSignal::slot_type &observer)
{
	return m_mainMenuPreShowSignal.connect(observer);
}

void Explorerplusplus::OnInitMenu(HMENU menu)
{
	// Note that as per
	// https://stackoverflow.com/questions/69917594/wm-initmenu-has-unexpected-wparam-for-system-menu#comment123596433_69917594,
	// the menu parameter passed to WM_INITMENU will be the main menu, even if the user is selecting
	// an item from the system menu. Additionally, WM_INITMENU will be sent simply when clicking a
	// blank spot the menu bar.
	// That can result in some unnecessary work (to update the state of the main menu), but
	// shouldn't have any functional issues. When an item is right-clicked, for example, the handle
	// to the menu the click occurred on will be passed in and that can be used to determine whether
	// or not the click should be processed.
	if (menu == GetMenu(m_hwnd))
	{
		SetMainMenuItemStates(menu);

		m_mainMenuPreShowSignal(menu);
		m_mainMenuShowing = true;
	}
}

void Explorerplusplus::OnEnterMenuLoop(bool shortcutMenu)
{
	if (!shortcutMenu)
	{
		NotifyTopLevelMenuShown();
	}
}

void Explorerplusplus::OnExitMenuLoop(bool shortcutMenu)
{
	if (!shortcutMenu)
	{
		NotifyTopLevelMenuClosed();

		m_mainMenuShowing = false;
	}
}

void Explorerplusplus::OnInitMenuPopup(HMENU menu)
{
	auto subMenu = std::ranges::find_if(m_mainMenuSubMenus,
		[menu](const auto &currentSubMenu) { return currentSubMenu.menuHost->GetMenu() == menu; });

	if (subMenu == m_mainMenuSubMenus.end())
	{
		return;
	}

	subMenu->menuHost->OnSubMenuWillShow();
}

bool Explorerplusplus::MaybeHandleMainMenuItemSelection(UINT id)
{
	auto *subMenu = MaybeGetMainMenuSubMenuFromId(id);

	if (!subMenu)
	{
		return false;
	}

	subMenu->menuHost->SelectItem(id, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT));

	return true;
}

boost::signals2::connection Explorerplusplus::AddMainMenuItemMiddleClickedObserver(
	const MainMenuItemMiddleClickedSignal::slot_type &observer)
{
	return m_mainMenuItemMiddleClickedSignal.connect(observer);
}

void Explorerplusplus::OnMenuMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	if (!m_mainMenuShowing)
	{
		return;
	}

	m_mainMenuItemMiddleClickedSignal(pt, isCtrlKeyDown, isShiftKeyDown);
}

boost::signals2::connection Explorerplusplus::AddMainMenuItemRightClickedObserver(
	const MainMenuItemRightClickedSignal::slot_type &observer)
{
	return m_mainMenuItemRightClickedSignal.connect(observer);
}

void Explorerplusplus::OnMenuRightButtonUp(HMENU menu, int index, const POINT &pt)
{
	if (!m_mainMenuShowing)
	{
		return;
	}

	m_mainMenuItemRightClickedSignal(menu, index, pt);
}

Explorerplusplus::MainMenuSubMenu *Explorerplusplus::MaybeGetMainMenuSubMenuFromId(UINT id)
{
	auto subMenu = std::ranges::find_if(m_mainMenuSubMenus,
		[id](const auto &currentSubMenu)
		{
			return id >= currentSubMenu.menu->GetIdRange().startId
				&& id < currentSubMenu.menu->GetIdRange().endId;
		});

	if (subMenu == m_mainMenuSubMenus.end())
	{
		return nullptr;
	}

	return &*subMenu;
}

void Explorerplusplus::SetMainMenuItemStates(HMENU mainMenu)
{
	const Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();

	ViewMode viewMode = tab.GetShellBrowser()->GetViewMode();

	int numSelected = tab.GetShellBrowserImpl()->GetNumSelected();
	bool anySelected = (numSelected > 0);

	MenuHelper::EnableItem(mainMenu, IDM_FILE_COPYITEMPATH,
		m_commandController.IsCommandEnabled(IDM_FILE_COPYITEMPATH));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_COPYUNIVERSALFILEPATHS,
		m_commandController.IsCommandEnabled(IDM_FILE_COPYUNIVERSALFILEPATHS));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_SETFILEATTRIBUTES,
		m_commandController.IsCommandEnabled(IDM_FILE_SETFILEATTRIBUTES));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_OPENCOMMANDPROMPT,
		m_commandController.IsCommandEnabled(IDM_FILE_OPENCOMMANDPROMPT));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR,
		m_commandController.IsCommandEnabled(IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_SAVEDIRECTORYLISTING,
		m_commandController.IsCommandEnabled(IDM_FILE_SAVEDIRECTORYLISTING));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_COPYCOLUMNTEXT,
		anySelected && (viewMode == +ViewMode::Details));

	MenuHelper::EnableItem(mainMenu, IDM_FILE_RENAME,
		m_commandController.IsCommandEnabled(IDM_FILE_RENAME));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_DELETE,
		m_commandController.IsCommandEnabled(IDM_FILE_DELETE));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_DELETEPERMANENTLY,
		m_commandController.IsCommandEnabled(IDM_FILE_DELETEPERMANENTLY));
	MenuHelper::EnableItem(mainMenu, IDM_FILE_PROPERTIES,
		m_commandController.IsCommandEnabled(IDM_FILE_PROPERTIES));

	MenuHelper::EnableItem(mainMenu, IDM_EDIT_UNDO, m_fileActionHandler.CanUndo());
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_PASTE, CanPaste(PasteType::Normal));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_PASTESHORTCUT, CanPaste(PasteType::Shortcut));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_PASTEHARDLINK, CanPasteLink());
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_PASTE_SYMBOLIC_LINK, CanPasteLink());

	MenuHelper::EnableItem(mainMenu, IDM_EDIT_CUT,
		m_commandController.IsCommandEnabled(IDM_EDIT_CUT));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_COPY,
		m_commandController.IsCommandEnabled(IDM_EDIT_COPY));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_MOVETOFOLDER,
		m_commandController.IsCommandEnabled(IDM_EDIT_MOVETOFOLDER));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_COPYTOFOLDER,
		m_commandController.IsCommandEnabled(IDM_EDIT_COPYTOFOLDER));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_WILDCARDDESELECT,
		m_commandController.IsCommandEnabled(IDM_EDIT_WILDCARDDESELECT));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_SELECTNONE,
		m_commandController.IsCommandEnabled(IDM_EDIT_SELECTNONE));
	MenuHelper::EnableItem(mainMenu, IDM_EDIT_RESOLVELINK, anySelected);

	if (m_featureList->IsEnabled(Feature::DualPane))
	{
		MenuHelper::CheckItem(mainMenu, IDM_VIEW_DUAL_PANE, m_config->dualPane);
	}

	MenuHelper::CheckItem(mainMenu, IDM_VIEW_STATUSBAR, m_config->showStatusBar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_FOLDERS, m_config->showFolders.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_DISPLAYWINDOW, m_config->showDisplayWindow.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_ADDRESS_BAR, m_config->showAddressBar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_MAIN_TOOLBAR,
		m_config->showMainToolbar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_BOOKMARKS_TOOLBAR,
		m_config->showBookmarksToolbar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_DRIVES_TOOLBAR,
		m_config->showDrivesToolbar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_APPLICATION_TOOLBAR,
		m_config->showApplicationToolbar.get());
	MenuHelper::CheckItem(mainMenu, IDM_VIEW_TOOLBARS_LOCK_TOOLBARS, m_config->lockToolbars.get());

	MenuHelper::EnableItem(mainMenu, IDM_VIEW_DECREASE_TEXT_SIZE,
		m_commandController.IsCommandEnabled(IDM_VIEW_DECREASE_TEXT_SIZE));
	MenuHelper::EnableItem(mainMenu, IDM_VIEW_INCREASE_TEXT_SIZE,
		m_commandController.IsCommandEnabled(IDM_VIEW_INCREASE_TEXT_SIZE));

	MenuHelper::CheckItem(mainMenu, IDM_VIEW_SHOWHIDDENFILES,
		tab.GetShellBrowserImpl()->GetShowHidden());
	MenuHelper::CheckItem(mainMenu, IDM_FILTER_ENABLE_FILTER,
		tab.GetShellBrowserImpl()->IsFilterEnabled());

	MenuHelper::EnableItem(mainMenu, IDM_ACTIONS_NEWFOLDER,
		m_commandController.IsCommandEnabled(IDM_ACTIONS_NEWFOLDER));
	MenuHelper::EnableItem(mainMenu, IDM_ACTIONS_SPLITFILE,
		m_commandController.IsCommandEnabled(IDM_ACTIONS_SPLITFILE));
	MenuHelper::EnableItem(mainMenu, IDM_ACTIONS_MERGEFILES,
		m_commandController.IsCommandEnabled(IDM_ACTIONS_MERGEFILES));
	MenuHelper::EnableItem(mainMenu, IDM_ACTIONS_DESTROYFILES, anySelected);

	UINT itemToCheck = GetViewModeMenuId(viewMode);
	CheckMenuRadioItem(mainMenu, IDM_VIEW_EXTRALARGEICONS, IDM_VIEW_TILES, itemToCheck,
		MF_BYCOMMAND);

	MenuHelper::EnableItem(mainMenu, IDM_GO_BACK,
		m_commandController.IsCommandEnabled(IDM_GO_BACK));
	MenuHelper::EnableItem(mainMenu, IDM_GO_FORWARD,
		m_commandController.IsCommandEnabled(IDM_GO_FORWARD));
	MenuHelper::EnableItem(mainMenu, IDM_GO_UP, m_commandController.IsCommandEnabled(IDM_GO_UP));

	MenuHelper::EnableItem(mainMenu, IDM_VIEW_AUTOSIZECOLUMNS,
		m_commandController.IsCommandEnabled(IDM_VIEW_AUTOSIZECOLUMNS));

	if (viewMode == +ViewMode::Details)
	{
		MenuHelper::EnableItem(mainMenu, IDM_VIEW_AUTOARRANGE, FALSE);
		MenuHelper::CheckItem(mainMenu, IDM_VIEW_AUTOARRANGE, FALSE);

		MenuHelper::EnableItem(mainMenu, IDM_VIEW_GROUPBY, TRUE);
	}
	else if (viewMode == +ViewMode::List)
	{
		MenuHelper::EnableItem(mainMenu, IDM_VIEW_GROUPBY, FALSE);

		MenuHelper::EnableItem(mainMenu, IDM_VIEW_AUTOARRANGE, FALSE);
		MenuHelper::CheckItem(mainMenu, IDM_VIEW_AUTOARRANGE, FALSE);
	}
	else
	{
		MenuHelper::EnableItem(mainMenu, IDM_VIEW_GROUPBY, TRUE);

		MenuHelper::EnableItem(mainMenu, IDM_VIEW_AUTOARRANGE, TRUE);
		MenuHelper::CheckItem(mainMenu, IDM_VIEW_AUTOARRANGE,
			tab.GetShellBrowser()->IsAutoArrangeEnabled());
	}

	SortMenuBuilder sortMenuBuilder(m_resourceLoader);
	auto [sortByMenu, groupByMenu] = sortMenuBuilder.BuildMenus(tab);

	MenuHelper::AttachSubMenu(mainMenu, std::move(sortByMenu), IDM_VIEW_SORTBY, FALSE);
	MenuHelper::AttachSubMenu(mainMenu, std::move(groupByMenu), IDM_VIEW_GROUPBY, FALSE);
}
