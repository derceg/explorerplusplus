// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "FrequentLocationsModel.h"
#include "FrequentLocationsTracker.h"
#include "MenuViewFake.h"
#include "MenuViewFakeTestHelper.h"
#include "ShellIconLoaderFake.h"
#include "ShellTestHelper.h"
#include <gtest/gtest.h>

class FrequentLocationsMenuTest : public BrowserTestBase
{
protected:
	FrequentLocationsMenuTest() :
		m_frequentLocationsModel(m_platformContext.GetSystemClock()),
		m_frequentLocationsTracker(&m_frequentLocationsModel, &m_navigationEvents),
		m_browser(AddBrowser()),
		m_menu(&m_menuView, &m_acceleratorManager, &m_frequentLocationsModel, m_browser,
			&m_shellIconLoader)
	{
	}

	FrequentLocationsModel m_frequentLocationsModel;
	FrequentLocationsTracker m_frequentLocationsTracker;
	ShellIconLoaderFake m_shellIconLoader;

	BrowserWindowFake *const m_browser;

	MenuViewFake m_menuView;
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

	MenuViewFakeTestHelper::CheckItemDetails(&m_menuView,
		{ CreateSimplePidlForTest(path3), CreateSimplePidlForTest(path1),
			CreateSimplePidlForTest(path2) });
}
