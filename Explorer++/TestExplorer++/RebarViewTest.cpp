// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RebarView.h"
#include "RebarBandStorageData.h"
#include "RebarViewTestApi.h"
#include "TestRebarBandView.h"
#include <gtest/gtest.h>
#include <wil/resource.h>
#include <memory>

using namespace testing;

class RebarViewTest : public Test
{
protected:
	void SetUp() override
	{
		m_parentWindow.reset(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_parentWindow, nullptr);

		m_view = std::make_unique<RebarView>(m_parentWindow.get(), WS_CHILD);
		m_testApi = std::make_unique<RebarViewTestApi>(m_view.get());
	}

	wil::unique_hwnd m_parentWindow;
	std::unique_ptr<RebarView> m_view;
	std::unique_ptr<RebarViewTestApi> m_testApi;
};

TEST_F(RebarViewTest, AddBand)
{
	UINT idCounter = 100;

	UINT id1 = idCounter++;
	TestRebarBandView bandView1(m_view->GetHWND());
	m_view->AddBand(id1, &bandView1, RebarView::OverflowMenuPolicy::Disabled);
	EXPECT_EQ(m_testApi->GetNumBands(), 1);
	EXPECT_EQ(m_testApi->GetBandIndex(id1), 0);

	UINT id2 = idCounter++;
	TestRebarBandView bandView2(m_view->GetHWND());
	m_view->AddBand(id2, &bandView2, RebarView::OverflowMenuPolicy::Enabled);
	EXPECT_EQ(m_testApi->GetNumBands(), 2);
	EXPECT_EQ(m_testApi->GetBandIndex(id2), 1);
}

TEST_F(RebarViewTest, OverflowMenuPolicy)
{
	UINT idCounter = 100;

	UINT id1 = idCounter++;
	TestRebarBandView bandView1(m_view->GetHWND());
	m_view->AddBand(id1, &bandView1, RebarView::OverflowMenuPolicy::Disabled);
	EXPECT_FALSE(m_testApi->DoesBandUseChevron(id1));

	UINT id2 = idCounter++;
	TestRebarBandView bandView2(m_view->GetHWND());
	m_view->AddBand(id2, &bandView2, RebarView::OverflowMenuPolicy::Enabled);
	EXPECT_TRUE(m_testApi->DoesBandUseChevron(id2));
}

TEST_F(RebarViewTest, BandHeight)
{
	UINT id = 100;

	TestRebarBandView bandView(m_view->GetHWND());
	bandView.SetHeight(70);
	m_view->AddBand(id, &bandView, RebarView::OverflowMenuPolicy::Disabled);
	EXPECT_EQ(m_testApi->GetBandMinimumHeight(id), 70u);

	bandView.SetHeight(160);
	EXPECT_EQ(m_testApi->GetBandMinimumHeight(id), 160u);
}

TEST_F(RebarViewTest, BandIdealWidth)
{
	UINT id = 100;

	TestRebarBandView bandView(m_view->GetHWND());
	bandView.SetIdealWidth(50);
	m_view->AddBand(id, &bandView, RebarView::OverflowMenuPolicy::Disabled);
	EXPECT_EQ(m_testApi->GetBandIdealWidth(id), 50u);

	bandView.SetIdealWidth(220);
	EXPECT_EQ(m_testApi->GetBandIdealWidth(id), 220u);
}

TEST_F(RebarViewTest, ShowBand)
{
	UINT id = 100;

	TestRebarBandView bandView(m_view->GetHWND());
	m_view->AddBand(id, &bandView, RebarView::OverflowMenuPolicy::Disabled);

	m_view->ShowBand(id, false);
	EXPECT_FALSE(m_testApi->IsBandShown(id));

	m_view->ShowBand(id, true);
	EXPECT_TRUE(m_testApi->IsBandShown(id));
}

TEST_F(RebarViewTest, SetBandOnNewLine)
{
	UINT idCounter = 100;

	UINT id1 = idCounter++;
	TestRebarBandView bandView1(m_view->GetHWND());
	m_view->AddBand(id1, &bandView1, RebarView::OverflowMenuPolicy::Disabled);

	UINT id2 = idCounter++;
	TestRebarBandView bandView2(m_view->GetHWND());
	m_view->AddBand(id2, &bandView2, RebarView::OverflowMenuPolicy::Disabled);

	m_view->SetBandOnNewLine(id2, false);
	EXPECT_FALSE(m_testApi->IsBandOnNewLine(id2));

	m_view->SetBandOnNewLine(id2, true);
	EXPECT_TRUE(m_testApi->IsBandOnNewLine(id2));
}

TEST_F(RebarViewTest, SetBandWidth)
{
	UINT id = 100;

	TestRebarBandView bandView(m_view->GetHWND());
	m_view->AddBand(id, &bandView, RebarView::OverflowMenuPolicy::Disabled);

	m_view->SetBandWidth(id, 200);
	EXPECT_EQ(m_testApi->GetBandWidth(id), 200u);
}

TEST_F(RebarViewTest, LockBands)
{
	UINT idCounter = 100;

	UINT id1 = idCounter++;
	TestRebarBandView bandView1(m_view->GetHWND());
	m_view->AddBand(id1, &bandView1, RebarView::OverflowMenuPolicy::Disabled);

	UINT id2 = idCounter++;
	TestRebarBandView bandView2(m_view->GetHWND());
	m_view->AddBand(id2, &bandView2, RebarView::OverflowMenuPolicy::Disabled);

	UINT id3 = idCounter++;
	TestRebarBandView bandView3(m_view->GetHWND());
	m_view->AddBand(id3, &bandView3, RebarView::OverflowMenuPolicy::Disabled);

	m_view->LockBands(true);
	EXPECT_TRUE(m_testApi->AreBandsLocked());

	// If the bands are locked, there shouldn't be a gripper.
	EXPECT_FALSE(m_testApi->IsBandGripperEnabled(id1));
	EXPECT_FALSE(m_testApi->IsBandGripperEnabled(id2));
	EXPECT_FALSE(m_testApi->IsBandGripperEnabled(id3));

	UINT id4 = idCounter++;
	TestRebarBandView bandView4(m_view->GetHWND());
	m_view->AddBand(id4, &bandView4, RebarView::OverflowMenuPolicy::Disabled);

	// A band that's added while the rebar is locked should also have no gripper.
	EXPECT_FALSE(m_testApi->IsBandGripperEnabled(id4));

	m_view->LockBands(false);
	EXPECT_FALSE(m_testApi->AreBandsLocked());

	// If the bands are unlocked, a gripper should be added to each band.
	EXPECT_TRUE(m_testApi->IsBandGripperEnabled(id1));
	EXPECT_TRUE(m_testApi->IsBandGripperEnabled(id2));
	EXPECT_TRUE(m_testApi->IsBandGripperEnabled(id3));
	EXPECT_TRUE(m_testApi->IsBandGripperEnabled(id4));
}

TEST_F(RebarViewTest, GetStorageData)
{
	UINT idCounter = 100;

	UINT id1 = idCounter++;
	TestRebarBandView bandView1(m_view->GetHWND());
	m_view->AddBand(id1, &bandView1, RebarView::OverflowMenuPolicy::Disabled);
	m_view->SetBandWidth(id1, 100);

	UINT id2 = idCounter++;
	TestRebarBandView bandView2(m_view->GetHWND());
	m_view->AddBand(id2, &bandView2, RebarView::OverflowMenuPolicy::Disabled);
	m_view->SetBandOnNewLine(id2, false);

	UINT id3 = idCounter++;
	TestRebarBandView bandView3(m_view->GetHWND());
	m_view->AddBand(id3, &bandView3, RebarView::OverflowMenuPolicy::Disabled);

	auto storageBands = m_view->GetStorageData();
	EXPECT_THAT(storageBands,
		ElementsAre(RebarBandStorageData{ id1, RBBS_BREAK, 100 }, RebarBandStorageData{ id2, 0, 0 },
			RebarBandStorageData{ id3, RBBS_BREAK, 0 }));
}
