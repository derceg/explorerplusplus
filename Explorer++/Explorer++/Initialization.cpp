// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Bookmarks/BookmarkTreeFactory.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "ColorRule.h"
#include "ColorRuleModel.h"
#include "ColorRuleModelFactory.h"
#include "Config.h"
#include "DarkModeHelper.h"
#include "DisplayWindow/DisplayWindow.h"
#include "Explorer++_internal.h"
#include "LoadSaveInterface.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "MainWindow.h"
#include "MenuRanges.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ViewModes.h"
#include "Tab.h"
#include "TabContainer.h"
#include "TaskbarThumbnails.h"
#include "ThemeManager.h"
#include "ThemeWindowTracker.h"
#include "UiTheming.h"
#include "ViewModeHelper.h"
#include "../Helper/iDirectoryMonitor.h"

/*
 * Main window creation.
 *
 * Settings are loaded very early on. Any initial settings must be in place before this.
 */
void Explorerplusplus::OnCreate()
{
	InitializeDefaultColorRules();

	LoadAllSettings();

	if (m_app->GetCommandLineSettings()->shellChangeNotificationType)
	{
		m_config->shellChangeNotificationType =
			*m_app->GetCommandLineSettings()->shellChangeNotificationType;
	}

	m_iconResourceLoader = std::make_unique<IconResourceLoader>(m_config->iconSet);

	SetLanguageModule();

	DarkModeHelper::GetInstance().EnableForApp(ShouldEnableDarkMode(m_config->theme.get()));
	m_config->theme.addObserver(std::bind_front(&Explorerplusplus::OnThemeUpdated, this));

	m_bookmarksMainMenu = std::make_unique<BookmarksMainMenu>(this, this, &m_iconFetcher,
		BookmarkTreeFactory::GetInstance()->GetBookmarkTree(),
		BookmarkMenuBuilder::MenuIdRange{ MENU_BOOKMARK_START_ID, MENU_BOOKMARK_END_ID });

	m_mainWindow = MainWindow::Create(m_hContainer, m_config, m_resourceInstance, this);

	InitializeMainMenu();

	CreateDirectoryMonitor(&m_pDirMon);

	CreateStatusBar();
	CreateMainRebarAndChildren();
	InitializeDisplayWindow();
	InitializeTabs();
	CreateFolderControls();

	/* All child windows MUST be resized before
	any listview changes take place. If auto arrange
	is turned off in the listview, when it is
	initially sized, all current items will lock
	to the current width. The only was to unlock
	them from this width is to turn auto arrange back on.
	Therefore, the listview MUST be set to the correct
	size initially. */
	UpdateLayout();

	m_taskbarThumbnails = TaskbarThumbnails::Create(this, GetActivePane()->GetTabContainer(),
		m_resourceInstance, m_config);

	CreateInitialTabs();

	// Register for any shell changes. This should be done after the tabs have
	// been created.
	SHChangeNotifyEntry shcne;
	shcne.fRecursive = TRUE;
	shcne.pidl = nullptr;
	m_SHChangeNotifyID = SHChangeNotifyRegister(m_hContainer, SHCNRF_ShellLevel, SHCNE_ASSOCCHANGED,
		WM_APP_ASSOCCHANGED, 1, &shcne);

	SetFocus(m_hActiveListView);

	m_uiTheming = std::make_unique<UiTheming>(this, GetActivePane()->GetTabContainer());

	InitializePlugins();

	m_themeWindowTracker = std::make_unique<ThemeWindowTracker>(m_hContainer);

	SetTimer(m_hContainer, AUTOSAVE_TIMER_ID, AUTOSAVE_TIMEOUT, nullptr);

	m_applicationInitialized = true;
	m_applicationInitializedSignal();
}

void Explorerplusplus::InitializeDefaultColorRules()
{
	auto *colorRuleModel = ColorRuleModelFactory::GetInstance()->GetColorRuleModel();

	colorRuleModel->AddItem(std::make_unique<ColorRule>(
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_GENERAL_COLOR_RULE_COMPRESSED),
		L"", false, FILE_ATTRIBUTE_COMPRESSED, RGB(0, 116, 232)));

	colorRuleModel->AddItem(std::make_unique<ColorRule>(
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_GENERAL_COLOR_RULE_ENCRYPTED), L"",
		false, FILE_ATTRIBUTE_ENCRYPTED, RGB(0, 128, 0)));
}

void Explorerplusplus::InitializeDisplayWindow()
{
	DWInitialSettings_t initialSettings;
	initialSettings.CentreColor = m_config->displayWindowCentreColor;
	initialSettings.SurroundColor = m_config->displayWindowSurroundColor;
	initialSettings.TextColor = m_config->displayWindowTextColor;
	initialSettings.hFont = m_config->displayWindowFont;
	initialSettings.hIcon = (HICON) LoadImage(GetModuleHandle(nullptr),
		MAKEINTRESOURCE(IDI_DISPLAYWINDOW), IMAGE_ICON, 0, 0, LR_CREATEDIBSECTION);

	m_hDisplayWindow = CreateDisplayWindow(m_hContainer, &initialSettings);

	ApplyDisplayWindowPosition();
}

wil::unique_hmenu Explorerplusplus::BuildViewsMenu()
{
	wil::unique_hmenu viewsMenu(CreatePopupMenu());
	AddViewModesToMenu(viewsMenu.get(), 0, TRUE);

	const Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	ViewMode currentViewMode = tab.GetShellBrowser()->GetViewMode();

	CheckMenuRadioItem(viewsMenu.get(), IDM_VIEW_EXTRALARGEICONS, IDM_VIEW_TILES,
		GetViewModeMenuId(currentViewMode), MF_BYCOMMAND);

	return viewsMenu;
}

void Explorerplusplus::AddViewModesToMenu(HMENU menu, UINT startPosition, BOOL byPosition)
{
	UINT position = startPosition;

	for (auto viewMode : VIEW_MODES)
	{
		std::wstring text = GetViewModeMenuText(viewMode, m_resourceInstance);

		MENUITEMINFO itemInfo;
		itemInfo.cbSize = sizeof(itemInfo);
		itemInfo.fMask = MIIM_ID | MIIM_STRING;
		itemInfo.wID = GetViewModeMenuId(viewMode);
		itemInfo.dwTypeData = text.data();
		InsertMenuItem(menu, position, byPosition, &itemInfo);

		if (byPosition)
		{
			position++;
		}
	}
}

bool Explorerplusplus::ShouldEnableDarkMode(Theme theme)
{
	return theme == +Theme::Dark
		|| (theme == +Theme::System && !DarkModeHelper::GetInstance().IsSystemAppModeLight());
}

boost::signals2::connection Explorerplusplus::AddApplicationInitializatedObserver(
	const ApplicationInitializedSignal::slot_type &observer)
{
	return m_applicationInitializedSignal.connect(observer);
}
