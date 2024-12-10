// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsMenu.h"
#include "AcceleratorManager.h"
#include "BrowserWindowMock.h"
#include "FrequentLocationsModel.h"
#include "PopupMenuView.h"
#include "ShellIconLoaderFake.h"
#include "ShellTestHelper.h"
#include "../Helper/SystemClockImpl.h"
#include <gtest/gtest.h>

using namespace testing;

class FrequentLocationsMenuTest : public Test
{
protected:
	FrequentLocationsMenuTest() :
		m_frequentLocationsModel(&m_systemClock),
		m_menu(&m_popupMenu, &m_acceleratorManager, &m_frequentLocationsModel, &m_browserWindow,
			&m_shellIconLoader)
	{
	}

	void CheckItemDetails(const std::vector<PidlAbsolute> &expectedItems)
	{
		ASSERT_EQ(static_cast<size_t>(m_popupMenu.GetItemCountForTesting()), expectedItems.size());

		for (size_t i = 0; i < expectedItems.size(); i++)
		{
			std::wstring name;
			HRESULT hr = GetDisplayName(expectedItems[i].Raw(), SHGDN_NORMAL, name);
			ASSERT_HRESULT_SUCCEEDED(hr);

			std::wstring path;
			hr = GetDisplayName(expectedItems[i].Raw(), SHGDN_FORPARSING, path);
			ASSERT_HRESULT_SUCCEEDED(hr);

			auto id = m_popupMenu.GetItemIdForTesting(static_cast<int>(i));
			EXPECT_EQ(m_popupMenu.GetItemTextForTesting(id), name);
			EXPECT_EQ(m_popupMenu.GetHelpTextForItem(id), path);
		}
	}

	PopupMenuView m_popupMenu;
	AcceleratorManager m_acceleratorManager;
	SystemClockImpl m_systemClock;
	FrequentLocationsModel m_frequentLocationsModel;
	BrowserWindowMock m_browserWindow;
	ShellIconLoaderFake m_shellIconLoader;
	FrequentLocationsMenu m_menu;
};

TEST_F(FrequentLocationsMenuTest, CheckItems)
{
	PidlAbsolute fake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	m_frequentLocationsModel.RegisterLocationVisit(fake1);
	m_frequentLocationsModel.RegisterLocationVisit(fake1);

	PidlAbsolute fake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	m_frequentLocationsModel.RegisterLocationVisit(fake2);

	PidlAbsolute fake3 = CreateSimplePidlForTest(L"C:\\Fake3");
	m_frequentLocationsModel.RegisterLocationVisit(fake3);
	m_frequentLocationsModel.RegisterLocationVisit(fake3);
	m_frequentLocationsModel.RegisterLocationVisit(fake3);

	CheckItemDetails({ fake3, fake1, fake2 });
}
