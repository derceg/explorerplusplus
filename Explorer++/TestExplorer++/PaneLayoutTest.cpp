// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "PaneLayout.h"
#include <gtest/gtest.h>
#include <limits>

using namespace testing;

class PaneLayoutTest : public Test
{
protected:
	PaneLayoutSettings m_settings = { .availableWidth = 1000,
		.availableHeight = 600,
		.treeViewPreferredWidth = 389,
		.treeViewMinimumWidth = 70,
		.displayWindowPreferredWidth = 300,
		.displayWindowMinimumWidth = 70,
		.displayWindowPreferredHeight = 90,
		.displayWindowMinimumHeight = 70,
		.showFolders = true };
};

TEST_F(PaneLayoutTest, PreferredSizesFit)
{
	auto sizes = CalculatePaneSizes(m_settings);
	EXPECT_EQ(sizes.treeViewWidth, 389);
	EXPECT_EQ(sizes.displayWindowWidth, 300);
	EXPECT_EQ(sizes.displayWindowHeight, 90);
}

TEST_F(PaneLayoutTest, FoldersPaneRecoversAfterTemporaryNarrowing)
{
	m_settings.availableWidth = 200;
	EXPECT_LT(CalculatePaneSizes(m_settings).treeViewWidth, 389);
	EXPECT_EQ(m_settings.treeViewPreferredWidth, 389);

	m_settings.availableWidth = 1000;
	EXPECT_EQ(CalculatePaneSizes(m_settings).treeViewWidth, 389);
}

TEST_F(PaneLayoutTest, DisplayWindowRecoversAfterTemporaryNarrowing)
{
	m_settings.availableWidth = 400;
	EXPECT_LT(CalculatePaneSizes(m_settings).displayWindowWidth, 300);
	EXPECT_EQ(m_settings.displayWindowPreferredWidth, 300);

	m_settings.availableWidth = 1000;
	EXPECT_EQ(CalculatePaneSizes(m_settings).displayWindowWidth, 300);
}

TEST_F(PaneLayoutTest, DisplayWindowRecoversAfterTemporaryHeightReduction)
{
	m_settings.availableHeight = 100;
	EXPECT_LT(CalculatePaneSizes(m_settings).displayWindowHeight, 90);
	EXPECT_EQ(m_settings.displayWindowPreferredHeight, 90);

	m_settings.availableHeight = 600;
	EXPECT_EQ(CalculatePaneSizes(m_settings).displayWindowHeight, 90);
}

TEST_F(PaneLayoutTest, OversizedPanesLeaveRoomForFileList)
{
	m_settings.treeViewPreferredWidth = std::numeric_limits<int>::max();
	m_settings.displayWindowPreferredWidth = std::numeric_limits<int>::max();
	m_settings.displayWindowPreferredHeight = std::numeric_limits<int>::max();

	auto sizes = CalculatePaneSizes(m_settings);
	EXPECT_GT(sizes.treeViewWidth, 0);
	EXPECT_GT(sizes.displayWindowWidth, 0);
	EXPECT_LT(sizes.treeViewWidth + sizes.displayWindowWidth, m_settings.availableWidth);
	EXPECT_LT(sizes.displayWindowHeight, m_settings.availableHeight);
}

TEST_F(PaneLayoutTest, HiddenFoldersDoNotReduceDisplayWindowSpace)
{
	m_settings.treeViewPreferredWidth = 800;
	m_settings.displayWindowPreferredWidth = 700;
	EXPECT_LT(CalculatePaneSizes(m_settings).displayWindowWidth, 700);

	m_settings.showFolders = false;
	EXPECT_EQ(CalculatePaneSizes(m_settings).displayWindowWidth, 700);
}

TEST_F(PaneLayoutTest, ScaledMinimumSizes)
{
	m_settings.treeViewPreferredWidth = 10;
	m_settings.displayWindowPreferredWidth = 10;
	m_settings.displayWindowPreferredHeight = 10;
	m_settings.treeViewMinimumWidth = 140;
	m_settings.displayWindowMinimumWidth = 140;
	m_settings.displayWindowMinimumHeight = 140;

	auto sizes = CalculatePaneSizes(m_settings);
	EXPECT_EQ(sizes.treeViewWidth, 140);
	EXPECT_EQ(sizes.displayWindowWidth, 140);
	EXPECT_EQ(sizes.displayWindowHeight, 140);
}

TEST_F(PaneLayoutTest, SmallWindowsTakePriorityOverMinimumSizes)
{
	m_settings.availableWidth = 100;
	m_settings.availableHeight = 100;
	m_settings.treeViewMinimumWidth = 140;
	m_settings.displayWindowMinimumWidth = 140;
	m_settings.displayWindowMinimumHeight = 140;

	auto sizes = CalculatePaneSizes(m_settings);
	EXPECT_GE(sizes.treeViewWidth, 0);
	EXPECT_GE(sizes.displayWindowWidth, 0);
	EXPECT_LE(sizes.treeViewWidth + sizes.displayWindowWidth, 100);
	EXPECT_GE(sizes.displayWindowHeight, 0);
	EXPECT_LE(sizes.displayWindowHeight, 100);
}

TEST_F(PaneLayoutTest, NoAvailableSpace)
{
	m_settings.availableWidth = 0;
	m_settings.availableHeight = 0;
	auto sizes = CalculatePaneSizes(m_settings);
	EXPECT_EQ(sizes.treeViewWidth, 0);
	EXPECT_EQ(sizes.displayWindowWidth, 0);
	EXPECT_EQ(sizes.displayWindowHeight, 0);
}

TEST_F(PaneLayoutTest, ToolbarsAndTabsCanExceedWindowHeight)
{
	m_settings.availableHeight = -100;
	EXPECT_EQ(CalculatePaneSizes(m_settings).displayWindowHeight, 0);
}

TEST_F(PaneLayoutTest, NegativeSavedSizes)
{
	m_settings.treeViewPreferredWidth = -1;
	m_settings.displayWindowPreferredWidth = -1;
	m_settings.displayWindowPreferredHeight = -1;
	auto sizes = CalculatePaneSizes(m_settings);
	EXPECT_EQ(sizes.treeViewWidth, 70);
	EXPECT_EQ(sizes.displayWindowWidth, 70);
	EXPECT_EQ(sizes.displayWindowHeight, 70);
}
