// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Application.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "GlobalHistoryMenu.h"
#include "HistoryServiceFactory.h"
#include "MainFontSetter.h"
#include "MainMenuSubMenuView.h"
#include "MainRebarStorage.h"
#include "MainResource.h"
#include "MenuRanges.h"
#include "Plugins/PluginManager.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowserHistoryHelper.h"
#include "TabRestorer.h"
#include "TabRestorerMenu.h"
#include "TabStorage.h"
#include "TaskbarThumbnails.h"
#include "ThemeWindowTracker.h"
#include "UiTheming.h"
#include "WindowStorage.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclassWrapper.h"
#include "../Helper/iDirectoryMonitor.h"

Explorerplusplus *Explorerplusplus::Create(App *app)
{
	auto defaultBounds = GetDefaultWindowBounds();
	return Explorerplusplus::Create(app, &defaultBounds, WindowShowState::Normal);
}

Explorerplusplus *Explorerplusplus::Create(App *app, const RECT *initialBounds,
	WindowShowState showState)
{
	return new Explorerplusplus(app, initialBounds, showState);
}

Explorerplusplus::Explorerplusplus(App *app, const RECT *initialBounds, WindowShowState showState) :
	m_app(app),
	m_hContainer(CreateMainWindow(initialBounds)),
	m_browserTracker(app->GetBrowserList(), this),
	m_commandController(this),
	m_tabBarBackgroundBrush(CreateSolidBrush(TAB_BAR_DARK_MODE_BACKGROUND_COLOR)),
	m_pluginMenuManager(m_hContainer, MENU_PLUGIN_START_ID, MENU_PLUGIN_END_ID),
	m_acceleratorUpdater(app->GetAcceleratorManager()),
	m_pluginCommandManager(app->GetAcceleratorManager(), ACCELERATOR_PLUGIN_START_ID,
		ACCELERATOR_PLUGIN_END_ID),
	m_config(app->GetConfig()),
	m_iconFetcher(m_hContainer, m_app->GetCachedIcons()),
	m_shellIconLoader(&m_iconFetcher)
{
	m_resourceInstance = nullptr;

	m_bSavePreferencesToXMLFile = FALSE;
	m_bLanguageLoaded = false;
	m_bShowTabBar = true;
	m_pActiveShellBrowser = nullptr;
	m_hMainRebar = nullptr;
	m_hStatusBar = nullptr;
	m_hTabBacking = nullptr;
	m_hTabWindowToolbar = nullptr;
	m_hDisplayWindow = nullptr;
	m_lastActiveWindow = nullptr;
	m_hActiveListView = nullptr;

	m_iDWFolderSizeUniqueId = 0;

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(m_hContainer,
		std::bind_front(&Explorerplusplus::WindowProcedure, this)));

	Initialize();

	if (showState == +WindowShowState::Minimized)
	{
		showState = WindowShowState::Normal;
	}

	ShowWindow(m_hContainer, ShowStateToNativeShowState(showState));
	UpdateWindow(m_hContainer);
}

Explorerplusplus::~Explorerplusplus()
{
	m_pDirMon->Release();
}

HWND Explorerplusplus::CreateMainWindow(const RECT *initialBounds)
{
	static bool mainWindowClassRegistered = false;

	if (!mainWindowClassRegistered)
	{
		LONG res = RegisterMainWindowClass(GetModuleHandle(nullptr));
		CHECK(res);

		mainWindowClassRegistered = true;
	}

	RECT validatedBounds = GetValidatedWindowBounds(initialBounds);

	HWND hwnd = CreateWindow(NExplorerplusplus::CLASS_NAME, NExplorerplusplus::APP_NAME,
		WS_OVERLAPPEDWINDOW, validatedBounds.left, validatedBounds.top,
		GetRectWidth(&validatedBounds), GetRectHeight(&validatedBounds), nullptr, nullptr,
		GetModuleHandle(nullptr), nullptr);
	CHECK(hwnd);

	return hwnd;
}

RECT Explorerplusplus::GetValidatedWindowBounds(const RECT *requestedBounds)
{
	// When shown in its normal size, the window should at least be on screen somewhere, even if
	// it's not completely visible.
	HMONITOR monitor = MonitorFromRect(requestedBounds, MONITOR_DEFAULTTONULL);

	if (!monitor)
	{
		return GetDefaultWindowBounds();
	}

	return *requestedBounds;
}

RECT Explorerplusplus::GetDefaultWindowBounds()
{
	RECT workArea;
	BOOL res = SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
	CHECK(res);

	// The strategy here is fairly simple - the window will be sized to a portion of the work area
	// of the primary monitor and centered.
	auto width = static_cast<int>(GetRectWidth(&workArea) * 0.60);
	auto height = static_cast<int>(GetRectHeight(&workArea) * 0.60);
	int x = (GetRectWidth(&workArea) - width) / 2;
	int y = (GetRectHeight(&workArea) - height) / 2;

	return { x, y, x + width, y + height };
}

ATOM Explorerplusplus::RegisterMainWindowClass(HINSTANCE instance)
{
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(windowClass);
	windowClass.style = 0;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = instance;
	windowClass.hIcon = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_MAIN),
		IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
	windowClass.hIconSm = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_MAIN),
		IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = nullptr;
	windowClass.lpszMenuName = nullptr;
	windowClass.lpszClassName = NExplorerplusplus::CLASS_NAME;
	return RegisterClassEx(&windowClass);
}

HWND Explorerplusplus::GetHWND() const
{
	return m_hContainer;
}

WindowStorageData Explorerplusplus::GetStorageData() const
{
	WINDOWPLACEMENT placement = {};
	placement.length = sizeof(placement);
	BOOL res = GetWindowPlacement(m_hContainer, &placement);
	CHECK(res);

	return WindowStorageData(placement.rcNormalPosition,
		NativeShowStateToShowState(placement.showCmd));
}

BrowserCommandController *Explorerplusplus::GetCommandController()
{
	return &m_commandController;
}

BrowserPane *Explorerplusplus::GetActivePane() const
{
	return m_browserPane.get();
}

ShellBrowser *Explorerplusplus::GetActiveShellBrowser()
{
	return GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowser();
}

void Explorerplusplus::OnShellBrowserCreated(ShellBrowser *shellBrowser)
{
	ShellBrowserHistoryHelper::CreateAndAttachToShellBrowser(shellBrowser,
		HistoryServiceFactory::GetInstance()->GetHistoryService());
}
