// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AppInfo.h"
#include "AppServices.h"
#include "BrowserList.h"
#include "BrowserView.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "HolderWindow.h"
#include "MainMenu.h"
#include "MainRebarStorage.h"
#include "MainRebarView.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellTreeView/ShellTreeView.h"
#include "StatusBar.h"
#include "StatusBarView.h"
#include "TabRestorer.h"
#include "TabStorage.h"
#include "ThemeWindowTracker.h"
#include "WindowMenuView.h"
#include "WindowStorage.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"
#include <fmt/format.h>
#include <fmt/xchar.h>

Explorerplusplus *Explorerplusplus::Create(const WindowStorageData &storageData,
	AppServices *appServices, HINSTANCE resourceInstance)
{
	return new Explorerplusplus(storageData, appServices, resourceInstance);
}

Explorerplusplus::Explorerplusplus(const WindowStorageData &storageData, AppServices *appServices,
	HINSTANCE resourceInstance) :
	m_appServices(appServices),
	m_resourceInstance(resourceInstance),
	m_config(appServices->GetConfig()),
	m_platformContext(appServices->GetPlatformContext()),
	m_acceleratorManager(appServices->GetAcceleratorManager()),
	m_featureList(appServices->GetFeatureList()),
	m_browserList(appServices->GetBrowserList()),
	m_tabEvents(appServices->GetTabEvents()),
	m_shellBrowserEvents(appServices->GetShellBrowserEvents()),
	m_navigationEvents(appServices->GetNavigationEvents()),
	m_resourceLoader(appServices->GetResourceLoader()),
	m_hwnd(CreateMainWindow(storageData)),
	m_commandController(this, appServices),
	m_tabBarBackgroundBrush(CreateSolidBrush(TAB_BAR_DARK_MODE_BACKGROUND_COLOR)),
	m_shellBrowserFactory(this, appServices, resourceInstance, &m_fileActionHandler),
	m_taskbarTabManager(this, m_appServices,
		[]() { return wil::CoCreateInstanceNoThrow<ITaskbarList4>(CLSID_TaskbarList); }),
	m_iconFetcher(m_hwnd, appServices->GetCachedIcons()),
	m_shellIconLoader(&m_iconFetcher),
	m_applicationExecutor(this)
{
	m_bShowTabBar = true;
	m_pActiveShellBrowser = nullptr;
	m_lastActiveWindow = nullptr;
	m_hActiveListView = nullptr;

	m_iDWFolderSizeUniqueId = 0;

	m_treeViewWidth = storageData.treeViewWidth;
	m_displayWindowWidth = storageData.displayWindowWidth;
	m_displayWindowHeight = storageData.displayWindowHeight;

	SetUpLayoutConfigListeners();

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hwnd,
		std::bind_front(&Explorerplusplus::WindowProcedure, this)));

	Initialize(storageData);

	WindowShowState showState = storageData.showState;

	if (showState == WindowShowState::Minimized)
	{
		showState = WindowShowState::Normal;
	}

	ShowWindow(m_hwnd, ShowStateToNativeShowState(showState));
	UpdateWindow(m_hwnd);

	m_browserList->AddBrowser(this);
}

Explorerplusplus::~Explorerplusplus() = default;

HWND Explorerplusplus::CreateMainWindow(const WindowStorageData &storageData)
{
	static bool mainWindowClassRegistered = false;

	if (!mainWindowClassRegistered)
	{
		auto res = RegisterMainWindowClass(GetModuleHandle(nullptr));
		CHECK(res);

		mainWindowClassRegistered = true;
	}

	HWND hwnd = CreateWindow(WINDOW_CLASS_NAME, AppInfo::NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(nullptr),
		nullptr);
	CHECK(hwnd);

	WINDOWPLACEMENT placement = {};
	placement.length = sizeof(placement);
	BOOL res = GetWindowPlacement(hwnd, &placement);
	CHECK(res);

	placement.showCmd = SW_HIDE;
	placement.rcNormalPosition = storageData.bounds;
	res = SetWindowPlacement(hwnd, &placement);
	CHECK(res);

	return hwnd;
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

void Explorerplusplus::SetUpLayoutConfigListeners()
{
	m_connections.push_back(
		m_config->showStatusBar.addObserver(std::bind(&Explorerplusplus::UpdateLayout, this)));
	m_connections.push_back(
		m_config->showFolders.addObserver(std::bind(&Explorerplusplus::UpdateLayout, this)));
	m_connections.push_back(
		m_config->showDisplayWindow.addObserver(std::bind(&Explorerplusplus::UpdateLayout, this)));
	m_connections.push_back(m_config->displayWindowVertical.addObserver(
		std::bind(&Explorerplusplus::UpdateLayout, this)));
}

void Explorerplusplus::Initialize(const WindowStorageData &storageData)
{
	m_view = BrowserView::Create(m_hwnd, this, m_config, m_tabEvents, m_shellBrowserEvents,
		m_navigationEvents, m_resourceLoader);

	InitializeMainMenu();

	auto *statusBarView = StatusBarView::Create(m_hwnd, m_config);
	m_statusBar = StatusBar::Create(statusBarView, this, m_config, m_tabEvents,
		m_shellBrowserEvents, m_navigationEvents, m_resourceLoader);

	CreateMainRebarAndChildren(storageData);
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

	CreateInitialTabs(storageData);

	SetFocus(m_hActiveListView);

	m_themeWindowTracker =
		std::make_unique<ThemeWindowTracker>(m_hwnd, m_appServices->GetThemeManager());

	SetLifecycleState(LifecycleState::Main);
}

void Explorerplusplus::InitializeMainMenu()
{
	m_mainMenuView = std::make_unique<WindowMenuView>(m_hwnd, this);
	m_mainMenu = std::make_unique<MainMenu>(m_mainMenuView.get(), this, m_appServices,
		&m_shellIconLoader, &m_iconFetcher, &m_fileActionHandler, this);

	// The menu is built after being assigned to the window, so the menu bar needs to be explicitly
	// redrawn.
	m_view->RedrawMenuBar();
}

void Explorerplusplus::InitializeDisplayWindow()
{
	m_displayWindow = DisplayWindow::Create(m_hwnd, DisplayWindow::Mode::Normal, this, m_config,
		m_acceleratorManager, m_resourceLoader);
}

void Explorerplusplus::CreateFolderControls()
{
	UINT holderStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (m_config->showFolders.get())
	{
		holderStyle |= WS_VISIBLE;
	}

	m_treeViewHolder = HolderWindow::Create(m_hwnd,
		m_resourceLoader->LoadString(IDS_FOLDERS_WINDOW_TEXT), holderStyle,
		m_resourceLoader->LoadString(IDS_HIDE_FOLDERS_PANE), m_config, m_resourceLoader,
		m_appServices->GetDarkModeManager(), m_appServices->GetDarkModeColorProvider());
	m_treeViewHolder->SetCloseButtonClickedCallback(
		[this]() { m_config->showFolders = !m_config->showFolders.get(); });
	m_treeViewHolder->SetResizedCallback(
		std::bind_front(&Explorerplusplus::OnTreeViewHolderResized, this));

	m_shellTreeView = ShellTreeView::Create(m_treeViewHolder->GetHWND(), this, m_appServices,
		&m_fileActionHandler);
	m_treeViewHolder->SetContentChild(m_shellTreeView->GetHWND());
}

void Explorerplusplus::OnTreeViewHolderResized(int newWidth)
{
	m_treeViewWidth = newWidth;

	UpdateLayout();
}

Tab *Explorerplusplus::CreateTabFromPreservedTab(const PreservedTab *tab)
{
	return &GetActivePane()->GetTabContainer()->CreateNewTab(*tab);
}

HWND Explorerplusplus::GetHWND() const
{
	return m_hwnd;
}

WindowStorageData Explorerplusplus::GetStorageData() const
{
	WINDOWPLACEMENT placement = {};
	placement.length = sizeof(placement);
	BOOL res = GetWindowPlacement(m_hwnd, &placement);
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
	return GetActiveWindow() == m_hwnd;
}

void Explorerplusplus::Activate()
{
	BringWindowToForeground(m_hwnd);
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

	std::wstring message =
		fmt::format(fmt::runtime(m_resourceLoader->LoadString(IDS_CLOSE_ALL_TABS)),
			fmt::arg(L"num_tabs", numTabs));
	int response =
		MessageBox(m_hwnd, message.c_str(), AppInfo::NAME, MB_ICONINFORMATION | MB_YESNO);

	if (response == IDNO)
	{
		return false;
	}

	return true;
}

void Explorerplusplus::Close()
{
	if (GetLifecycleState() != LifecycleState::Main)
	{
		return;
	}

	BeginShutdown();

	// When the last tab is closed, the window will be destroyed.
	GetActiveTabContainer()->CloseAllTabs();
}

void Explorerplusplus::BeginShutdown()
{
	if (GetLifecycleState() != LifecycleState::Main)
	{
		return;
	}

	SetLifecycleState(LifecycleState::WillClose);

	m_browserList->WillRemoveBrowser(this);

	// Past this point, the remaining tabs will be closed, which can cause other UI updates. There's
	// no need for that to be shown, however, since the window will be destroyed shortly afterwards.
	ShowWindow(m_hwnd, SW_HIDE);
}

void Explorerplusplus::FinishShutdown()
{
	SetLifecycleState(LifecycleState::Closing);

	m_browserList->RemoveBrowser(this);

	DestroyWindow(m_hwnd);
}

BrowserCommandController *Explorerplusplus::GetCommandController()
{
	return &m_commandController;
}

BrowserPane *Explorerplusplus::GetActivePane() const
{
	return m_browserPane.get();
}

TabContainer *Explorerplusplus::GetActiveTabContainer()
{
	return GetActivePane()->GetTabContainer();
}

const TabContainer *Explorerplusplus::GetActiveTabContainer() const
{
	return GetActivePane()->GetTabContainer();
}

ShellBrowser *Explorerplusplus::GetActiveShellBrowser()
{
	return GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowser();
}

const ShellBrowser *Explorerplusplus::GetActiveShellBrowser() const
{
	return GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowser();
}

void Explorerplusplus::StartMainToolbarCustomization()
{
	m_mainToolbar->StartCustomization();
}

void Explorerplusplus::NotifyTopLevelMenuShown()
{
	m_numTopLevelMenusActive++;

	if (m_numTopLevelMenusActive == 1)
	{
		m_statusBar->NotifyMenuLoopStarted();
	}
}

void Explorerplusplus::NotifyTopLevelMenuClosed()
{
	m_numTopLevelMenusActive--;
	CHECK_GE(m_numTopLevelMenusActive, 0);

	if (m_numTopLevelMenusActive == 0)
	{
		m_statusBar->NotifyMenuLoopEnded();
	}
}

void Explorerplusplus::MenuItemSelected(HMENU menu, WORD indexOrId, UINT flags)
{
	if (!menu)
	{
		return;
	}

	// This can be an index or an ID (depending on whether item is a popup item), but
	// m_statusBar->OnMenuSelect() will only use the value if the item being selected is a non-popup
	// item. So, as far as that method it concerned, the value is an ID.
	UINT id = indexOrId;

	if (WI_AreAllFlagsClear(flags, MF_POPUP | MF_SEPARATOR))
	{
		// WM_MENUSELECT silently truncates the ID to 16-bit. As IDs here can be above that range,
		// the provided ID will be ignored and the highlighted item will be found instead.
		auto highlightedItemId = MenuHelper::MaybeGetHighlightedItem(menu);

		if (!highlightedItemId)
		{
			// This is invoked in response to WM_MENUSELECT, so there should be a highlighted item.
			DCHECK(false);
			return;
		}

		// The ID of the highlighted item should match the truncated ID that was provided.
		DCHECK_EQ(static_cast<WORD>(*highlightedItemId), indexOrId);

		id = *highlightedItemId;
	}

	m_statusBar->OnMenuSelect(menu, id, flags);
}

boost::signals2::connection Explorerplusplus::AddMenuHelpTextRequestObserver(
	const MenuHelpTextRequestSignal::slot_type &observer)
{
	return m_menuHelpTextRequestSignal.connect(observer);
}

std::optional<std::wstring> Explorerplusplus::RequestMenuHelpText(HMENU menu, UINT id) const
{
	auto helpText = m_menuHelpTextRequestSignal(menu, id);

	if (!helpText)
	{
		helpText = m_resourceLoader->MaybeLoadString(id);
	}

	return helpText;
}
