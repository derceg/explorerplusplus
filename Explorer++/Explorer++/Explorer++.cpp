// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Application.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "BrowserTracker.h"
#include "Config.h"
#include "FrequentLocationsMenu.h"
#include "FrequentLocationsShellBrowserHelper.h"
#include "HistoryMenu.h"
#include "HistoryShellBrowserHelper.h"
#include "MainFontSetter.h"
#include "MainMenuSubMenuView.h"
#include "MainRebarStorage.h"
#include "MainRebarView.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "MenuRanges.h"
#include "Plugins/PluginManager.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabRestorer.h"
#include "TabRestorerMenu.h"
#include "TabStorage.h"
#include "TaskbarThumbnails.h"
#include "ThemeWindowTracker.h"
#include "UiTheming.h"
#include "WindowStorage.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include "../Helper/iDirectoryMonitor.h"
#include <fmt/format.h>
#include <fmt/xchar.h>

Explorerplusplus *Explorerplusplus::Create(App *app, const WindowStorageData *storageData)
{
	return new Explorerplusplus(app, storageData);
}

Explorerplusplus::Explorerplusplus(App *app, const WindowStorageData *storageData) :
	m_id(idCounter++),
	m_app(app),
	m_hContainer(CreateMainWindow(storageData)),
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
	m_bShowTabBar = true;
	m_pActiveShellBrowser = nullptr;
	m_hStatusBar = nullptr;
	m_hTabBacking = nullptr;
	m_hTabWindowToolbar = nullptr;
	m_lastActiveWindow = nullptr;
	m_hActiveListView = nullptr;

	m_iDWFolderSizeUniqueId = 0;

	if (storageData)
	{
		m_treeViewWidth = storageData->treeViewWidth;
		m_displayWindowWidth = storageData->displayWindowWidth;
		m_displayWindowHeight = storageData->displayWindowHeight;
	}

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hContainer,
		std::bind_front(&Explorerplusplus::WindowProcedure, this)));

	Initialize(storageData);

	WindowShowState showState = storageData ? storageData->showState : +WindowShowState::Normal;

	if (showState == +WindowShowState::Minimized)
	{
		showState = WindowShowState::Normal;
	}

	ShowWindow(m_hContainer, ShowStateToNativeShowState(showState));
	UpdateWindow(m_hContainer);

	m_browserTracker = std::make_unique<BrowserTracker>(app->GetBrowserList(), this);
}

Explorerplusplus::~Explorerplusplus()
{
	m_pDirMon->Release();

	*m_destroyed = true;
}

HWND Explorerplusplus::CreateMainWindow(const WindowStorageData *storageData)
{
	bool isFirstInstance = IsFirstInstance();
	static bool mainWindowClassRegistered = false;

	if (!mainWindowClassRegistered)
	{
		auto res = RegisterMainWindowClass(GetModuleHandle(nullptr));
		CHECK(res);

		mainWindowClassRegistered = true;
	}

	HWND hwnd = CreateWindow(WINDOW_CLASS_NAME, App::APP_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(nullptr),
		nullptr);
	CHECK(hwnd);

	WINDOWPLACEMENT placement = {};
	placement.length = sizeof(placement);
	BOOL res = GetWindowPlacement(hwnd, &placement);
	CHECK(res);

	placement.showCmd = SW_HIDE;
	placement.rcNormalPosition =
		storageData && isFirstInstance ? storageData->bounds : GetDefaultMainWindowBounds();
	SetWindowPlacement(hwnd, &placement);

	return hwnd;
}

bool Explorerplusplus::IsFirstInstance()
{
	HWND hPrev = FindWindow(WINDOW_CLASS_NAME, nullptr);
	return (hPrev == nullptr);
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
	windowClass.lpszClassName = WINDOW_CLASS_NAME;
	return RegisterClassEx(&windowClass);
}

RECT Explorerplusplus::GetDefaultMainWindowBounds()
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

boost::signals2::connection Explorerplusplus::AddBrowserInitializedObserver(
	const BrowserInitializedSignal::slot_type &observer)
{
	return m_browserInitializedSignal.connect(observer);
}

void Explorerplusplus::CreateTabFromPreservedTab(const PreservedTab *tab)
{
	GetActivePane()->GetTabContainer()->CreateNewTab(*tab);
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

	const auto *tabContainer = GetActivePane()->GetTabContainer();

	return { .bounds = placement.rcNormalPosition,
		.showState = NativeShowStateToShowState(placement.showCmd),
		.tabs = tabContainer->GetStorageData(),
		.selectedTab = tabContainer->GetSelectedTabIndex(),
		.mainRebarInfo = m_mainRebarView->GetStorageData(),
		.mainToolbarButtons = m_mainToolbar->GetButtonsForStorage(),
		.treeViewWidth = m_treeViewWidth,
		.displayWindowWidth = m_displayWindowWidth,
		.displayWindowHeight = m_displayWindowHeight };
}

bool Explorerplusplus::IsActive() const
{
	return GetActiveWindow() == m_hContainer;
}

void Explorerplusplus::Activate()
{
	BringWindowToForeground(m_hContainer);
}

void Explorerplusplus::TryClose()
{
	if (!ConfirmClose())
	{
		return;
	}

	Close();
}

bool Explorerplusplus::ConfirmClose()
{
	if (!m_config->confirmCloseTabs)
	{
		return true;
	}

	auto numTabs = GetActivePane()->GetTabContainer()->GetNumTabs();

	if (numTabs == 1)
	{
		return true;
	}

	std::wstring message = fmt::format(
		fmt::runtime(ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_CLOSE_ALL_TABS)),
		fmt::arg(L"num_tabs", numTabs));
	int response =
		MessageBox(m_hContainer, message.c_str(), App::APP_NAME, MB_ICONINFORMATION | MB_YESNO);

	if (response == IDNO)
	{
		return false;
	}

	return true;
}

void Explorerplusplus::Close()
{
	m_browserTracker.reset();

	DestroyWindow(m_hContainer);
}

int Explorerplusplus::GetId() const
{
	return m_id;
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
	return GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowserImpl();
}

void Explorerplusplus::OnShellBrowserCreated(ShellBrowser *shellBrowser)
{
	HistoryShellBrowserHelper::CreateAndAttachToShellBrowser(shellBrowser,
		m_app->GetHistoryModel());
	FrequentLocationsShellBrowserHelper::CreateAndAttachToShellBrowser(shellBrowser,
		m_app->GetFrequentLocationsModel());
}
