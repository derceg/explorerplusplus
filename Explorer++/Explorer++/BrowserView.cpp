// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BrowserView.h"
#include "AddressBarView.h"
#include "AppInfo.h"
#include "AppServices.h"
#include "ApplicationToolbarView.h"
#include "Bookmarks/UI/Views/BookmarksToolbarView.h"
#include "BrowserPaneView.h"
#include "Config.h"
#include "DrivesToolbarView.h"
#include "MainRebarView.h"
#include "MainResource.h"
#include "MainToolbarView.h"
#include "ResourceLoader.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "StatusBarView.h"
#include "TabContainer.h"
#include "TabEvents.h"
#include "ThemeWindowTracker.h"
#include "WindowMenuView.h"
#include "../Helper/Helper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/WindowHelper.h"

BrowserView::BrowserView(BrowserWindow *browser, const RECT &bounds, AppServices *appServices) :
	ViewBase(CreateBrowserWindow(bounds)),
	m_subclass(GetHWND(), std::bind_front(&BrowserView::WndProc, this)),
	m_browser(browser),
	m_config(appServices->GetConfig()),
	m_themeManager(appServices->GetThemeManager()),
	m_resourceLoader(appServices->GetResourceLoader())
{
	m_connections.push_back(m_browser->AddLifecycleStateChangedObserver(
		std::bind_front(&BrowserView::OnBrowserLifecycleStateChanged, this)));

	m_connections.push_back(appServices->GetTabEvents()->AddSelectedObserver(
		std::bind_front(&BrowserView::OnTabSelected, this), TabEventScope::ForBrowser(*m_browser)));

	m_connections.push_back(
		appServices->GetShellBrowserEvents()->AddDirectoryPropertiesChangedObserver(
			std::bind_front(&BrowserView::OnDirectoryPropertiesChanged, this),
			NavigationEventScope::ForActiveShellBrowser(*m_browser)));

	m_connections.push_back(appServices->GetNavigationEvents()->AddCommittedObserver(
		std::bind_front(&BrowserView::OnNavigationCommitted, this),
		NavigationEventScope::ForActiveShellBrowser(*m_browser)));

	m_connections.push_back(m_config->showFullTitlePath.addObserver(
		std::bind_front(&BrowserView::OnShowFullTitlePathUpdated, this)));
	m_connections.push_back(m_config->showUserNameInTitleBar.addObserver(
		std::bind_front(&BrowserView::OnShowUserNameInTitleBarUpdated, this)));
	m_connections.push_back(m_config->showPrivilegeLevelInTitleBar.addObserver(
		std::bind_front(&BrowserView::OnShowPrivilegeLevelInTitleBarUpdated, this)));

	// The window is registered as a drop target only so that the drag image will be consistently
	// shown when an item is being dragged. For the drag image to be shown, the relevant
	// IDropTargetHelper methods need to be called during the drag. To do that, the window under the
	// mouse needs to be registered as a drop target.
	//
	// Rather than having to register every window, the top-level window can simply be registered
	// instead. That way, it will act as a fallback if there isn't a more specific child window
	// registered.
	m_dropTargetWindow =
		winrt::make_self<DropTargetWindow>(GetHWND(), static_cast<DropTargetInternal *>(this));

	m_mainMenuView = std::make_unique<WindowMenuView>(GetHWND(), m_browser);

	CreateMainRebarAndChildren();

	m_browserPaneView =
		std::make_unique<BrowserPaneView>(this, m_browser, appServices->GetDarkModeManager(),
			appServices->GetDarkModeColorProvider(), m_config, m_resourceLoader);

	m_statusBarView = std::make_unique<StatusBarView>(GetHWND(), m_config);
}

BrowserView::~BrowserView() = default;

HWND BrowserView::CreateBrowserWindow(const RECT &bounds)
{
	static bool browserWindowClassRegistered = false;

	if (!browserWindowClassRegistered)
	{
		auto res = RegisterBrowserWindowClass(GetModuleHandle(nullptr));
		CHECK(res);

		browserWindowClassRegistered = true;
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
	placement.rcNormalPosition = bounds;
	res = SetWindowPlacement(hwnd, &placement);
	CHECK(res);

	return hwnd;
}

ATOM BrowserView::RegisterBrowserWindowClass(HINSTANCE instance)
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

void BrowserView::CreateMainRebarAndChildren()
{
	m_mainRebarView = std::make_unique<MainRebarView>(GetHWND());
	m_mainToolbarView =
		std::make_unique<MainToolbarView>(m_mainRebarView->GetHWND(), m_browser, m_config);
	m_addressBarView = std::make_unique<AddressBarView>(m_mainRebarView->GetHWND(), m_config);
	m_bookmarksToolbarView =
		std::make_unique<BookmarksToolbarView>(m_mainRebarView->GetHWND(), m_browser, m_config);
	m_drivesToolbarView =
		std::make_unique<DrivesToolbarView>(m_mainRebarView->GetHWND(), m_browser, m_config);
	m_applicationToolbarView = std::make_unique<Applications::ApplicationToolbarView>(
		m_mainRebarView->GetHWND(), m_browser, m_config);
}

MenuView *BrowserView::GetMainMenuView() const
{
	return m_mainMenuView.get();
}

MainRebarView *BrowserView::GetMainRebarView() const
{
	return m_mainRebarView.get();
}

MainToolbarView *BrowserView::GetMainToolbarView() const
{
	return m_mainToolbarView.get();
}

AddressBarView *BrowserView::GetAddressBarView() const
{
	return m_addressBarView.get();
}

BookmarksToolbarView *BrowserView::GetBookmarksToolbarView() const
{
	return m_bookmarksToolbarView.get();
}

DrivesToolbarView *BrowserView::GetDrivesToolbarView() const
{
	return m_drivesToolbarView.get();
}

Applications::ApplicationToolbarView *BrowserView::GetApplicationToolbarView() const
{
	return m_applicationToolbarView.get();
}

BrowserPaneView *BrowserView::GetBrowserPaneView() const
{
	return m_browserPaneView.get();
}

StatusBarView *BrowserView::GetStatusBarView() const
{
	return m_statusBarView.get();
}

LRESULT BrowserView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DPICHANGED:
		OnDpiChanged(reinterpret_cast<RECT *>(lParam));
		return 0;

	// COM calls (such as IDropTarget::DragEnter) can result in a call being made to PeekMessage().
	// That method will then dispatch non-queued messages, with WM_CLOSE being one such message.
	//
	// That's an issue, as it means if a WM_CLOSE message is in the message queue when a COM method
	// is called, the WM_CLOSE message could be processed, the main window destroyed and
	// Explorerplusplus instance deleted, all within the call to the COM method. Once the COM method
	// returns, the application isn't going to be in a valid state and will crash.
	//
	// PeekMessage() won't, however, dispatch posted (i.e. queued) messages. So the message that's
	// posted here will only be processed in the normal message loop. If a COM modal loop is
	// running, the message won't be processed until that modal loop ends and the normal message
	// loop resumes.
	case WM_CLOSE:
		PostMessage(hwnd, WM_APP_CLOSE, 0, 0);
		return 0;

	case WM_APP_CLOSE:
		m_browser->TryClose();
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void BrowserView::OnDpiChanged(const RECT *updatedWindowRect)
{
	SetWindowPos(GetHWND(), nullptr, updatedWindowRect->left, updatedWindowRect->top,
		GetRectWidth(updatedWindowRect), GetRectHeight(updatedWindowRect),
		SWP_NOZORDER | SWP_NOACTIVATE);
}

void BrowserView::OnBrowserLifecycleStateChanged(BrowserWindow::LifecycleState updatedState)
{
	if (updatedState == BrowserWindow::LifecycleState::Main)
	{
		m_themeWindowTracker = std::make_unique<ThemeWindowTracker>(GetHWND(), m_themeManager);

		UpdateWindowText();
	}
}

void BrowserView::OnNavigationCommitted(const NavigationRequest *request)
{
	UNREFERENCED_PARAMETER(request);

	UpdateWindowText();
}

void BrowserView::OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser)
{
	UNREFERENCED_PARAMETER(shellBrowser);

	UpdateWindowText();
}

void BrowserView::OnTabSelected(const Tab &tab)
{
	UNREFERENCED_PARAMETER(tab);

	UpdateWindowText();
}

void BrowserView::OnShowFullTitlePathUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void BrowserView::OnShowUserNameInTitleBarUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void BrowserView::OnShowPrivilegeLevelInTitleBarUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void BrowserView::UpdateWindowText()
{
	if (m_browser->GetLifecycleState() != BrowserWindow::LifecycleState::Main)
	{
		return;
	}

	const Tab &tab = m_browser->GetActiveTabContainer()->GetSelectedTab();
	const auto &pidl = tab.GetShellBrowser()->GetDirectory();

	std::wstring folderDisplayName;

	/* Don't show full paths for virtual folders (as only the folders
	GUID will be shown). */
	if (m_config->showFullTitlePath.get() && !tab.GetShellBrowserImpl()->InVirtualFolder())
	{
		GetDisplayName(pidl.Raw(), SHGDN_FORPARSING, folderDisplayName);
	}
	else
	{
		GetDisplayName(pidl.Raw(), SHGDN_NORMAL, folderDisplayName);
	}

	std::wstring title = std::format(L"{} - {}", folderDisplayName, AppInfo::NAME);

	if (m_config->showUserNameInTitleBar.get() || m_config->showPrivilegeLevelInTitleBar.get())
	{
		title += L" [";
	}

	if (m_config->showUserNameInTitleBar.get())
	{
		TCHAR owner[512];
		GetProcessOwner(GetCurrentProcessId(), owner, std::size(owner));

		title += owner;
	}

	if (m_config->showPrivilegeLevelInTitleBar.get())
	{
		std::wstring privilegeLevel;

		if (CheckGroupMembership(GroupType::Administrators))
		{
			privilegeLevel = m_resourceLoader->LoadString(IDS_PRIVILEGE_LEVEL_ADMINISTRATORS);
		}
		else if (CheckGroupMembership(GroupType::PowerUsers))
		{
			privilegeLevel = m_resourceLoader->LoadString(IDS_PRIVILEGE_LEVEL_POWER_USERS);
		}
		else if (CheckGroupMembership(GroupType::Users))
		{
			privilegeLevel = m_resourceLoader->LoadString(IDS_PRIVILEGE_LEVEL_USERS);
		}
		else if (CheckGroupMembership(GroupType::UsersRestricted))
		{
			privilegeLevel = m_resourceLoader->LoadString(IDS_PRIVILEGE_LEVEL_USERS_RESTRICTED);
		}

		if (m_config->showUserNameInTitleBar.get())
		{
			title += L" - ";
		}

		title += privilegeLevel;
	}

	if (m_config->showUserNameInTitleBar.get() || m_config->showPrivilegeLevelInTitleBar.get())
	{
		title += L"]";
	}

	SetWindowText(GetHWND(), title.c_str());
}

void BrowserView::RedrawMenuBar()
{
	auto res = DrawMenuBar(GetHWND());
	CHECK(res);
}

// This is used for both Tab/Shift+Tab and F6/Shift+F6. While IsDialogMessage() could be used to
// handle Tab/Shift+Tab, the key combinations in this case are synonyms for each other. Since
// F6/Shift+F6 would need to be handled manually anyway, handling both with the same function
// ensures that they have identical behavior.
void BrowserView::FocusNextWindow(FocusChangeDirection direction)
{
	HWND focus = GetFocus();
	HWND initialControl;

	// The focus should always be on one of the child windows, but this function should still set
	// the focus even if there is no current focus or the focus is on the parent.
	//
	// GetNextDlgTabItem() may fail if the initial control is NULL or the parent window, so that
	// situation is prevented here.
	if (focus && IsChild(GetHWND(), focus))
	{
		initialControl = focus;
	}
	else
	{
		initialControl = GetWindow(GetHWND(), GW_CHILD);
	}

	HWND nextWindow =
		GetNextDlgTabItem(GetHWND(), initialControl, direction == FocusChangeDirection::Previous);
	DCHECK(nextWindow);

	if (nextWindow)
	{
		SetFocus(nextWindow);
	}
}

// DropTargetInternal
// Note that, as described above, this window is registered as a drop target only so that drag
// images are shown consistently. Dropping items isn't supported at all.
DWORD BrowserView::DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);
	UNREFERENCED_PARAMETER(effect);

	return DROPEFFECT_NONE;
}

DWORD BrowserView::DragOver(DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);
	UNREFERENCED_PARAMETER(effect);

	return DROPEFFECT_NONE;
}

void BrowserView::DragLeave()
{
}

DWORD BrowserView::Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);
	UNREFERENCED_PARAMETER(effect);

	return DROPEFFECT_NONE;
}
