// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MenuTestHost.h"
#include "MenuViewTestHelper.h"
#include "PidlTestHelper.h"
#include "ShellIconLoaderFake.h"
#include <gtest/gtest.h>

class FrequentLocationsMenuTest : public BrowserTestBase
{
protected:
	FrequentLocationsMenuTest() :
		m_browser(AddBrowser()),
		m_menu(m_menuHost.GetView(), &m_acceleratorManager, &m_frequentLocationsModel, m_browser,
			&m_shellIconLoader)
	{
	}

	ShellIconLoaderFake m_shellIconLoader;

	BrowserWindowFake *const m_browser;

	MenuTestHost m_menuHost;
	FrequentLocationsMenu m_menu;
};

TEST_F(FrequentLocationsMenuTest, CheckItems)
{
	std::wstring path1 = L"c:\\fake1";
	m_browser->AddTab(path1);
	m_browser->AddTab(path1);

	std::wstring path2 = L"c:\\fake2";
	m_browser->AddTab(path2);

	std::wstring path3 = L"c:\\fake3";
	m_browser->AddTab(path3);
	m_browser->AddTab(path3);
	m_browser->AddTab(path3);

	MenuViewTestHelper::CheckShellItemDetails(m_menuHost.GetView(),
		{ CreateSimplePidlForTest(path3), CreateSimplePidlForTest(path1),
			CreateSimplePidlForTest(path2) });
}
