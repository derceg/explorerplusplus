// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ListViewColumnsMenu.h"
#include "AcceleratorManager.h"
#include "GeneratorTestHelper.h"
#include "ListViewColumnModel.h"
#include "MenuTestHost.h"
#include "MenuViewTestHelper.h"
#include "ResourceLoaderFake.h"
#include <gtest/gtest.h>

using namespace testing;

class ListViewColumnsMenuTest : public Test
{
protected:
	static constexpr ListViewColumnId COLUMN_A{ 1 };
	static constexpr ListViewColumnId COLUMN_B{ 2 };
	static constexpr ListViewColumnId COLUMN_C{ 3 };

	ListViewColumnsMenuTest() :
		m_model(
			{ { COLUMN_A, 1, 100, true }, { COLUMN_B, 1, 100, true }, { COLUMN_C, 1, 100, true } },
			COLUMN_A),
		m_menu(m_menuHost.GetView(), &m_acceleratorManager, &m_model, &m_resourceLoader)
	{
	}

	AcceleratorManager m_acceleratorManager;
	ResourceLoaderFake m_resourceLoader;

	ListViewColumnModel m_model;

	MenuTestHost m_menuHost;
	ListViewColumnsMenu m_menu;
};

TEST_F(ListViewColumnsMenuTest, MenuItemStates)
{
	auto *menuView = m_menuHost.GetView();

	auto columnIds = GeneratorToVector(m_model.GetAllColumnIds());
	ASSERT_EQ(static_cast<size_t>(menuView->GetNumItems()), columnIds.size());

	for (int i = 0; i < menuView->GetNumItems(); i++)
	{
		// The primary column can't be removed, so its menu item should be disabled. All other items
		// should be enabled.
		auto columnId = columnIds[i];
		MenuViewTestHelper::ExpectItemEnabled(menuView, menuView->GetItemIdForTesting(i),
			!m_model.IsPrimaryColumnId(columnId));
	}
}

TEST_F(ListViewColumnsMenuTest, Selection)
{
	auto *menuView = m_menuHost.GetView();

	auto columnIds = GeneratorToVector(m_model.GetAllColumnIds());
	ASSERT_EQ(static_cast<size_t>(menuView->GetNumItems()), columnIds.size());

	for (int i = 0; i < menuView->GetNumItems(); i++)
	{
		m_menuHost.SelectItemAtIndex(i, false, false);

		// All of the columns should be toggled off, except for the primary column.
		auto columnId = columnIds[i];
		EXPECT_EQ(m_model.IsColumnVisible(columnId), m_model.IsPrimaryColumnId(columnId));
	}

	for (int i = 0; i < menuView->GetNumItems(); i++)
	{
		m_menuHost.SelectItemAtIndex(i, false, false);

		// The above call should have toggled the columns back on.
		auto columnId = columnIds[i];
		EXPECT_TRUE(m_model.IsColumnVisible(columnId));
	}
}
