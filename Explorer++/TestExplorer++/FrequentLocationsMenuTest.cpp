// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsMenu.h"
#include "AcceleratorManager.h"
#include "BrowserWindowMock.h"
#include "FrequentLocationsModel.h"
#include "PopupMenuView.h"
#include "PopupMenuViewTestHelper.h"
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

	PopupMenuViewTestHelper::CheckItemDetails(&m_popupMenu, { fake3, fake1, fake2 });
}
