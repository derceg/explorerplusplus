// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserWindowFake.h"
#include "MainRebarStorage.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellTestHelper.h"
#include "Tab.h"
#include "TabEvents.h"
#include "TabStorage.h"
#include "WindowStorage.h"

BrowserWindowFake::BrowserWindowFake(TabEvents *tabEvents, NavigationEvents *navigationEvents) :
	m_tabEvents(tabEvents),
	m_navigationEvents(navigationEvents),
	m_window(CreateBrowserWindow())
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

boost::signals2::connection BrowserWindowFake::AddBrowserInitializedObserver(
	const BrowserInitializedSignal::slot_type &observer)
{
	UNREFERENCED_PARAMETER(observer);

	return {};
}

BrowserCommandController *BrowserWindowFake::GetCommandController()
{
	return nullptr;
}

BrowserPane *BrowserWindowFake::GetActivePane() const
{
	return nullptr;
}

void BrowserWindowFake::FocusActiveTab()
{
}

void BrowserWindowFake::CreateTabFromPreservedTab(const PreservedTab *tab)
{
	UNREFERENCED_PARAMETER(tab);
}

ShellBrowser *BrowserWindowFake::GetActiveShellBrowser()
{
	CHECK(m_activeTabIndex < m_tabs.size());
	return m_tabs[m_activeTabIndex]->GetShellBrowser();
}

const ShellBrowser *BrowserWindowFake::GetActiveShellBrowser() const
{
	CHECK(m_activeTabIndex < m_tabs.size());
	return m_tabs[m_activeTabIndex]->GetShellBrowser();
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

void BrowserWindowFake::FocusChanged()
{
}

void BrowserWindowFake::TryClose()
{
}

void BrowserWindowFake::Close()
{
}

void BrowserWindowFake::OpenDefaultItem(OpenFolderDisposition openFolderDisposition)
{
	UNREFERENCED_PARAMETER(openFolderDisposition);
}

void BrowserWindowFake::OpenItem(const std::wstring &itemPath,
	OpenFolderDisposition openFolderDisposition)
{
	if (openFolderDisposition == OpenFolderDisposition::CurrentTab)
	{
		auto pidl = CreateSimplePidlForTest(itemPath);
		MaybeNavigateActiveShellBrowser(pidl.Raw());
	}
}

void BrowserWindowFake::OpenItem(PCIDLIST_ABSOLUTE pidlItem,
	OpenFolderDisposition openFolderDisposition)
{
	if (openFolderDisposition == OpenFolderDisposition::CurrentTab)
	{
		MaybeNavigateActiveShellBrowser(pidlItem);
	}
}

boost::signals2::connection BrowserWindowFake::AddMenuHelpTextRequestObserver(
	const MenuHelpTextRequestSignal::slot_type &observer)
{
	UNREFERENCED_PARAMETER(observer);

	return {};
}

Tab *BrowserWindowFake::AddTab()
{
	auto tab = std::make_unique<Tab>(
		std::make_unique<ShellBrowserFake>(m_navigationEvents, &m_tabNavigation), this, nullptr,
		m_tabEvents);
	auto *rawTab = tab.get();
	m_tabs.push_back(std::move(tab));

	m_tabEvents->NotifyCreated(*rawTab, m_activeTabIndex == m_tabs.size());

	return rawTab;
}

void BrowserWindowFake::ActivateTabAtIndex(size_t index)
{
	CHECK(index < m_tabs.size());
	m_activeTabIndex = index;
	m_tabEvents->NotifySelected(*m_tabs[m_activeTabIndex]);
}

void BrowserWindowFake::MaybeNavigateActiveShellBrowser(PCIDLIST_ABSOLUTE pidl)
{
	if (m_activeTabIndex >= m_tabs.size())
	{
		return;
	}

	auto navigateParams = NavigateParams::Normal(pidl);
	m_tabs[m_activeTabIndex]->GetShellBrowser()->GetNavigationController()->Navigate(
		navigateParams);
}
