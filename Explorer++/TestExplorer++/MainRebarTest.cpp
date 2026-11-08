// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MainRebar.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MainRebarView.h"
#include "RebarBand.h"
#include "RebarBandView.h"
#include "RebarViewTestApi.h"
#include "TestRebarBandView.h"
#include <gtest/gtest.h>

namespace
{

class TestRebarBand : public RebarBand
{
public:
	TestRebarBand(RebarBandView *view) : m_view(view)
	{
	}

	RebarBandView *GetView() override
	{
		return m_view;
	}

private:
	RebarBandView *const m_view;
};

enum class InitialShowState
{
	Show,
	Hide
};

struct RebarBandTestData
{
	RebarBandTestData(HWND parent, InitialShowState initialShowState = InitialShowState::Show) :
		id(idCounter++),
		view(parent),
		controller(&view),
		visible(initialShowState == InitialShowState::Show)
	{
	}

	static inline UINT idCounter = 100;
	UINT id;
	TestRebarBandView view;
	TestRebarBand controller;
	ValueWrapper<bool> visible;
};

}

class MainRebarTest : public BrowserTestBase
{
protected:
	MainRebarTest() : m_browser(AddBrowser())
	{
	}

	std::vector<MainRebar::BandDefinition> CreateDefinitionsForBands(
		const std::vector<RebarBandTestData *> &bands)
	{
		std::vector<MainRebar::BandDefinition> bandDefinitions;

		for (auto *band : bands)
		{
			bandDefinitions.push_back(
				{ band->id, &band->controller, &band->visible, ToolbarOptionsMenu::Source::Other });
		}

		return bandDefinitions;
	}

	BrowserWindowFake *const m_browser;
};

TEST_F(MainRebarTest, StorageData)
{
	MainRebarView mainRebarView(m_browser->GetView());

	RebarBandTestData band1(mainRebarView.GetHWND());
	RebarBandTestData band2(mainRebarView.GetHWND());
	RebarBandTestData band3(mainRebarView.GetHWND());
	RebarBandTestData band4(mainRebarView.GetHWND());

	std::vector<MainRebar::BandDefinition> bandDefinitions =
		CreateDefinitionsForBands({ &band1, &band2, &band3, &band4 });

	std::vector<RebarBandStorageData> storageInfo = {
		{ band4.id, RBBS_BREAK, 0 },
		{ band2.id, 0, 50 },
		{ band1.id, RBBS_BREAK, 100 },
		{ band3.id, 0, 0 },
	};

	MainRebar mainRebar(&mainRebarView, bandDefinitions, storageInfo, m_browser, &m_appServices);

	EXPECT_EQ(mainRebarView.GetStorageData(), storageInfo);
}

TEST_F(MainRebarTest, ShowState)
{
	MainRebarView mainRebarView(m_browser->GetView());

	RebarBandTestData band1(mainRebarView.GetHWND(), InitialShowState::Hide);
	RebarBandTestData band2(mainRebarView.GetHWND(), InitialShowState::Show);

	std::vector<MainRebar::BandDefinition> bandDefinitions =
		CreateDefinitionsForBands({ &band1, &band2 });

	MainRebar mainRebar(&mainRebarView, bandDefinitions, {}, m_browser, &m_appServices);

	RebarViewTestApi testApi(&mainRebarView);
	EXPECT_FALSE(testApi.IsBandShown(band1.id));
	EXPECT_TRUE(testApi.IsBandShown(band2.id));

	band1.visible = true;
	EXPECT_TRUE(testApi.IsBandShown(band1.id));
	EXPECT_TRUE(testApi.IsBandShown(band2.id));

	band2.visible = false;
	EXPECT_TRUE(testApi.IsBandShown(band1.id));
	EXPECT_FALSE(testApi.IsBandShown(band2.id));
}

TEST_F(MainRebarTest, LockToolbars)
{
	m_config.lockToolbars = true;

	MainRebarView mainRebarView(m_browser->GetView());
	MainRebar mainRebar(&mainRebarView, {}, {}, m_browser, &m_appServices);

	RebarViewTestApi testApi(&mainRebarView);
	EXPECT_TRUE(testApi.AreBandsLocked());

	m_config.lockToolbars = false;
	EXPECT_FALSE(testApi.AreBandsLocked());
}
