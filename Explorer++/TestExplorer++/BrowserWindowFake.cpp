// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserWindowFake.h"
#include "Config.h"
#include "MainRebarStorage.h"
#include "MainTabView.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellTestHelper.h"
#include "Tab.h"
#include "TabEvents.h"
#include "TabStorage.h"
#include "WindowStorage.h"

BrowserWindowFake::BrowserWindowFake(const Config *config, TabEvents *tabEvents,
	ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
	CachedIcons *cachedIcons, BookmarkTree *bookmarkTree,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader,
	PlatformContext *platformContext) :
	m_config(config),
	m_window(CreateBrowserWindow()),
	m_shellBrowserFactory(this, navigationEvents),
	m_tabContainer(TabContainer::Create(MainTabView::Create(m_window.get(), config, resourceLoader),
		this, &m_shellBrowserFactory, tabEvents, shellBrowserEvents, navigationEvents, nullptr,
		cachedIcons, bookmarkTree, acceleratorManager, config, resourceLoader, platformContext))
{
}

wil::unique_hwnd BrowserWindowFake::CreateBrowserWindow()
{
	static bool classRegistered = false;

	if (!classRegistered)
	{
		RegisterBrowserWindowClass();

		classRegistered = true;
	}

	wil::unique_hwnd hwnd(CreateWindow(CLASS_NAME, L"", WS_OVERLAPPEDWINDOW | WS_DISABLED,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr,
		GetModuleHandle(nullptr), nullptr));
	CHECK(hwnd);

	return hwnd;
}

void BrowserWindowFake::RegisterBrowserWindowClass()
{
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = CLASS_NAME;
	windowClass.hInstance = GetModuleHandle(nullptr);
	windowClass.style = 0;
	auto atom = RegisterClass(&windowClass);
	CHECK(atom);
}

HWND BrowserWindowFake::GetHWND() const
{
	return m_window.get();
}

BrowserCommandController *BrowserWindowFake::GetCommandController()
{
	return nullptr;
}

BrowserPane *BrowserWindowFake::GetActivePane() const
{
	return nullptr;
}

TabContainer *BrowserWindowFake::GetActiveTabContainer()
{
	return m_tabContainer;
}

const TabContainer *BrowserWindowFake::GetActiveTabContainer() const
{
	return m_tabContainer;
}

void BrowserWindowFake::FocusActiveTab()
{
}

Tab *BrowserWindowFake::CreateTabFromPreservedTab(const PreservedTab *tab)
{
	return &GetActiveTabContainer()->CreateNewTab(*tab);
}

void BrowserWindowFake::OpenDefaultItem(OpenFolderDisposition openFolderDisposition)
{
	UNREFERENCED_PARAMETER(openFolderDisposition);
}

void BrowserWindowFake::OpenItem(const std::wstring &itemPath,
	OpenFolderDisposition openFolderDisposition)
{
	auto pidl = CreateSimplePidlForTest(itemPath);
	OpenItem(pidl.Raw(), openFolderDisposition);
}

void BrowserWindowFake::OpenItem(PCIDLIST_ABSOLUTE pidlItem,
	OpenFolderDisposition openFolderDisposition)
{
	SFGAOF attributes = SFGAO_FOLDER | SFGAO_STREAM;
	HRESULT hr = GetItemAttributes(pidlItem, &attributes);

	if (FAILED(hr))
	{
		return;
	}

	if (WI_IsFlagClear(attributes, SFGAO_FOLDER)
		|| WI_AreAllFlagsSet(attributes, SFGAO_FOLDER | SFGAO_STREAM))
	{
		OpenFileItem(pidlItem, L"");
		return;
	}

	if (openFolderDisposition == OpenFolderDisposition::CurrentTab)
	{
		if (m_config->alwaysOpenNewTab)
		{
			openFolderDisposition = OpenFolderDisposition::ForegroundTab;
		}
	}
	else if (openFolderDisposition == OpenFolderDisposition::NewTabDefault)
	{
		openFolderDisposition = m_config->openTabsInForeground
			? OpenFolderDisposition::ForegroundTab
			: OpenFolderDisposition::BackgroundTab;
	}
	else if (openFolderDisposition == OpenFolderDisposition::NewTabAlternate)
	{
		openFolderDisposition = m_config->openTabsInForeground
			? OpenFolderDisposition::BackgroundTab
			: OpenFolderDisposition::ForegroundTab;
	}

	switch (openFolderDisposition)
	{
	case OpenFolderDisposition::CurrentTab:
	{
		auto navigateParams = NavigateParams::Normal(pidlItem);
		GetActiveShellBrowser()->GetNavigationController()->Navigate(navigateParams);
	}
	break;

	case OpenFolderDisposition::BackgroundTab:
	{
		auto navigateParams = NavigateParams::Normal(pidlItem);
		GetActiveTabContainer()->CreateNewTab(navigateParams);
	}
	break;

	case OpenFolderDisposition::ForegroundTab:
	{
		auto navigateParams = NavigateParams::Normal(pidlItem);
		GetActiveTabContainer()->CreateNewTab(navigateParams, { .selected = true });
	}
	break;

	// This case isn't handled yet.
	case OpenFolderDisposition::NewWindow:
		break;

	// These values are transformed above, so don't need to be handled here.
	case OpenFolderDisposition::NewTabDefault:
	case OpenFolderDisposition::NewTabAlternate:
		break;
	}
}

void BrowserWindowFake::OpenFileItem(const std::wstring &itemPath, const std::wstring &parameters)
{
	auto pidl = CreateSimplePidlForTest(itemPath);
	OpenFileItem(pidl.Raw(), parameters);
}

void BrowserWindowFake::OpenFileItem(PCIDLIST_ABSOLUTE pidlItem, const std::wstring &parameters)
{
	// No action is taken to open files here.
	UNREFERENCED_PARAMETER(pidlItem);
	UNREFERENCED_PARAMETER(parameters);
}

ShellBrowser *BrowserWindowFake::GetActiveShellBrowser()
{
	return GetActiveTabContainer()->GetSelectedTab().GetShellBrowser();
}

const ShellBrowser *BrowserWindowFake::GetActiveShellBrowser() const
{
	return GetActiveTabContainer()->GetSelectedTab().GetShellBrowser();
}

void BrowserWindowFake::StartMainToolbarCustomization()
{
}

std::optional<std::wstring> BrowserWindowFake::RequestMenuHelpText(HMENU menu, UINT id) const
{
	UNREFERENCED_PARAMETER(menu);
	UNREFERENCED_PARAMETER(id);

	return std::nullopt;
}

WindowStorageData BrowserWindowFake::GetStorageData() const
{
	return { {}, WindowShowState::Normal };
}

bool BrowserWindowFake::IsActive() const
{
	return false;
}

void BrowserWindowFake::Activate()
{
}

void BrowserWindowFake::TryClose()
{
}

void BrowserWindowFake::Close()
{
}

void BrowserWindowFake::MenuItemSelected(HMENU menu, UINT itemId, UINT flags)
{
	UNREFERENCED_PARAMETER(menu);
	UNREFERENCED_PARAMETER(itemId);
	UNREFERENCED_PARAMETER(flags);
}

boost::signals2::connection BrowserWindowFake::AddMenuHelpTextRequestObserver(
	const MenuHelpTextRequestSignal::slot_type &observer)
{
	UNREFERENCED_PARAMETER(observer);

	return {};
}

int BrowserWindowFake::AddTabAndReturnId(const std::wstring &path, const TabSettings &tabSettings,
	PidlAbsolute *outputPidl)
{
	const auto &tab = AddTab(path, tabSettings, outputPidl);
	return tab->GetId();
}

Tab *BrowserWindowFake::AddTab(const std::wstring &path, const TabSettings &tabSettings,
	PidlAbsolute *outputPidl)
{
	auto pidl = CreateSimplePidlForTest(path);
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	auto &tab = GetActiveTabContainer()->CreateNewTab(navigateParams, tabSettings);

	wil::assign_to_opt_param(outputPidl, pidl);

	return &tab;
}
