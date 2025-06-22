// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include <wil/common.h>

BrowserTestBase::BrowserTestBase() :
	m_cachedIcons(10),
	m_resourceLoader(GetModuleHandle(nullptr), IconSet::Color, nullptr, nullptr)
{
}

BrowserTestBase::~BrowserTestBase() = default;

BrowserWindowFake *BrowserTestBase::AddBrowser()
{
	auto browser = std::make_unique<BrowserWindowFake>(&m_config, &m_tabEvents,
		&m_shellBrowserEvents, &m_navigationEvents, &m_cachedIcons, &m_bookmarkTree,
		&m_acceleratorManager, &m_resourceLoader);
	auto *rawBrowser = browser.get();
	m_browsers.push_back(std::move(browser));
	return rawBrowser;
}

void BrowserTestBase::RemoveBrowser(const BrowserWindowFake *browser)
{
	auto itr = std::ranges::find_if(m_browsers,
		[browser](const auto &currentBrowser) { return currentBrowser.get() == browser; });
	ASSERT_TRUE(itr != m_browsers.end());
	m_browsers.erase(itr);
}

void BrowserTestBase::NavigateTab(Tab *tab, const std::wstring &path, PidlAbsolute *outputPidl)
{
	auto pidl = CreateSimplePidlForTest(path);
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	tab->GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);

	wil::assign_to_opt_param(outputPidl, pidl);
}
