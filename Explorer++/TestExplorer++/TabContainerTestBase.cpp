// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabContainerTestBase.h"
#include "BrowserWindowFake.h"
#include "IconSet.h"
#include "MainTabView.h"
#include "ShellTestHelper.h"

TabContainerTestBase::TabContainerTestBase() :
	m_resourceLoader(GetModuleHandle(nullptr), IconSet::Color, nullptr, nullptr),
	m_cachedIcons(10),
	m_browser(AddBrowser()),
	m_shellBrowserFactory(&m_navigationEvents, &m_tabNavigation),
	m_tabView(MainTabView::Create(m_browser->GetHWND(), &m_config, &m_resourceLoader)),
	m_tabContainer(TabContainer::Create(m_tabView, m_browser, &m_shellBrowserFactory, &m_tabEvents,
		&m_shellBrowserEvents, &m_navigationEvents, nullptr, &m_cachedIcons, &m_bookmarkTree,
		&m_acceleratorManager, &m_config, &m_resourceLoader))
{
}

int TabContainerTestBase::AddTabAndReturnId(const std::wstring &path,
	const TabSettings &tabSettings)
{
	const auto &tab = AddTab(path, tabSettings);
	return tab.GetId();
}

Tab &TabContainerTestBase::AddTab(const std::wstring &path, const TabSettings &tabSettings)
{
	auto pidl = CreateSimplePidlForTest(path);
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	return m_tabContainer->CreateNewTab(navigateParams, tabSettings);
}
