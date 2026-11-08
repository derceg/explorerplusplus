// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserWindowFake.h"
#include "AppServices.h"
#include "Config.h"
#include "PidlTestHelper.h"
#include "RebarBandStorageData.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "Tab.h"
#include "TabBarView.h"
#include "TabEvents.h"
#include "TabStorage.h"
#include "WindowStorage.h"

BrowserWindowFake::BrowserWindowFake(const WindowStorageData &storageData,
	AppServices *appServices) :
	m_view(storageData.bounds),
	m_config(appServices->GetConfig()),
	m_shellBrowserFactory(this, appServices->GetNavigationEvents()),
	m_tabBarView(&m_view, appServices->GetThemeManager(), appServices->GetConfig(),
		appServices->GetResourceLoader()),
	m_tabBar(&m_tabBarView, this, &m_shellBrowserFactory, appServices)
{
	for (const auto &tab : storageData.tabs)
	{
		if (tab.pidl.HasValue())
		{
			AddTab(tab.pidl, tab.tabSettings);
		}
		else
		{
			AddTab(tab.directory, tab.tabSettings);
		}
	}

	auto *tabBar = GetActiveTabBar();

	if (storageData.selectedTab >= 0 && storageData.selectedTab < tabBar->GetNumTabs())
	{
		tabBar->SelectTabAtIndex(storageData.selectedTab);
	}
}

BrowserWindowFakeView *BrowserWindowFake::GetView()
{
	return &m_view;
}

HWND BrowserWindowFake::GetHWND() const
{
	return m_view.GetHWND();
}

BrowserCommandController *BrowserWindowFake::GetCommandController()
{
	return nullptr;
}

BrowserPane *BrowserWindowFake::GetActivePane() const
{
	return nullptr;
}

TabBar *BrowserWindowFake::GetActiveTabBar()
{
	return &m_tabBar;
}

const TabBar *BrowserWindowFake::GetActiveTabBar() const
{
	return &m_tabBar;
}

void BrowserWindowFake::FocusActiveTab()
{
}

Tab *BrowserWindowFake::CreateTabFromPreservedTab(const PreservedTab *tab)
{
	return &GetActiveTabBar()->CreateNewTab(*tab);
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
		GetActiveTabBar()->CreateNewTab(navigateParams);
	}
	break;

	case OpenFolderDisposition::ForegroundTab:
	{
		auto navigateParams = NavigateParams::Normal(pidlItem);
		GetActiveTabBar()->CreateNewTab(navigateParams, { .selected = true });
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
	return GetActiveTabBar()->GetSelectedTab().GetShellBrowser();
}

const ShellBrowser *BrowserWindowFake::GetActiveShellBrowser() const
{
	return GetActiveTabBar()->GetSelectedTab().GetShellBrowser();
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
	WINDOWPLACEMENT placement = {};
	placement.length = sizeof(placement);
	BOOL res = GetWindowPlacement(m_view.GetHWND(), &placement);
	CHECK(res);

	const auto *tabBar = GetActiveTabBar();

	return { .bounds = placement.rcNormalPosition,
		.showState = NativeShowStateToShowState(placement.showCmd),
		.tabs = tabBar->GetStorageData(),
		.selectedTab = tabBar->GetSelectedTabIndex() };
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

void BrowserWindowFake::NotifyBrowserClosing()
{
	GetActiveTabBar()->CloseAllTabs();
}

void BrowserWindowFake::NotifyTopLevelMenuShown()
{
}

void BrowserWindowFake::NotifyTopLevelMenuClosed()
{
}

void BrowserWindowFake::MenuItemSelected(HMENU menu, WORD indexOrId, UINT flags)
{
	UNREFERENCED_PARAMETER(menu);
	UNREFERENCED_PARAMETER(indexOrId);
	UNREFERENCED_PARAMETER(flags);
}

boost::signals2::connection BrowserWindowFake::AddMenuHelpTextRequestObserver(
	const MenuHelpTextRequestSignal::slot_type &observer)
{
	UNREFERENCED_PARAMETER(observer);

	return {};
}

void BrowserWindowFake::SetWorkspaceBounds(const RECT &bounds)
{
	WINDOWPLACEMENT placement = {};
	placement.length = sizeof(placement);
	BOOL res = GetWindowPlacement(m_view.GetHWND(), &placement);
	CHECK(res);

	placement.showCmd = SW_HIDE;
	placement.rcNormalPosition = bounds;
	res = SetWindowPlacement(m_view.GetHWND(), &placement);
	CHECK(res);
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
	wil::assign_to_opt_param(outputPidl, pidl);
	return AddTab(pidl, tabSettings);
}

Tab *BrowserWindowFake::AddTab(const PidlAbsolute &pidl, const TabSettings &tabSettings)
{
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	auto &tab = GetActiveTabBar()->CreateNewTab(navigateParams, tabSettings);
	return &tab;
}
