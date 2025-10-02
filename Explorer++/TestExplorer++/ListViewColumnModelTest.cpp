// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ListViewColumnModel.h"
#include "GeneratorTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class ListViewColumnModelTest : public Test
{
protected:
	static constexpr ListViewColumnId COLUMN_A{ 1 };
	static constexpr ListViewColumnId COLUMN_B{ 2 };
	static constexpr ListViewColumnId COLUMN_C{ 3 };

	ListViewColumnModelTest() :
		m_initialColumns(
			{ { COLUMN_A, 1, 100, true }, { COLUMN_B, 1, 100, true }, { COLUMN_C, 1, 100, true } }),
		m_model(m_initialColumns, COLUMN_A)
	{
	}

	const std::vector<ListViewColumn> m_initialColumns;
	ListViewColumnModel m_model;
};

TEST_F(ListViewColumnModelTest, CopyConstructor)
{
	ListViewColumnModel model2(m_model);
	EXPECT_EQ(model2.GetPrimaryColumnId(), m_model.GetPrimaryColumnId());
	EXPECT_EQ(GeneratorToVector(model2.GetAllColumnIds()),
		GeneratorToVector(m_model.GetAllColumnIds()));
}

TEST_F(ListViewColumnModelTest, Assignment)
{
	ListViewColumnModel model2({ { COLUMN_B, 1, 40, true } }, COLUMN_B);
	model2 = m_model;
	EXPECT_EQ(model2.GetPrimaryColumnId(), m_model.GetPrimaryColumnId());
	EXPECT_EQ(GeneratorToVector(model2.GetAllColumnIds()),
		GeneratorToVector(m_model.GetAllColumnIds()));
}

TEST_F(ListViewColumnModelTest, GetAllColumnIds)
{
	EXPECT_THAT(GeneratorToVector(m_model.GetAllColumnIds()),
		ElementsAre(COLUMN_A, COLUMN_B, COLUMN_C));
}

TEST_F(ListViewColumnModelTest, GetVisibleColumnIds)
{
	EXPECT_THAT(GeneratorToVector(m_model.GetVisibleColumnIds()),
		ElementsAre(COLUMN_A, COLUMN_B, COLUMN_C));

	m_model.SetColumnVisible(COLUMN_C, false);
	EXPECT_THAT(GeneratorToVector(m_model.GetVisibleColumnIds()), ElementsAre(COLUMN_A, COLUMN_B));

	m_model.SetColumnVisible(COLUMN_B, false);
	EXPECT_THAT(GeneratorToVector(m_model.GetVisibleColumnIds()), ElementsAre(COLUMN_A));
}

TEST_F(ListViewColumnModelTest, GetNumVisibleColumns)
{
	EXPECT_EQ(m_model.GetNumVisibleColumns(), 3);

	m_model.SetColumnVisible(COLUMN_C, false);
	EXPECT_EQ(m_model.GetNumVisibleColumns(), 2);

	m_model.SetColumnVisible(COLUMN_B, false);
	EXPECT_EQ(m_model.GetNumVisibleColumns(), 1);
}

TEST_F(ListViewColumnModelTest, MaybeGetColumnVisibleIndex)
{
	EXPECT_EQ(m_model.MaybeGetColumnVisibleIndex(COLUMN_B), 1);

	m_model.SetColumnVisible(COLUMN_B, false);
	EXPECT_EQ(m_model.MaybeGetColumnVisibleIndex(COLUMN_B), std::nullopt);
}

TEST_F(ListViewColumnModelTest, GetColumnIdAtVisibleIndex)
{
	EXPECT_EQ(m_model.GetColumnIdAtVisibleIndex(0), COLUMN_A);
	EXPECT_EQ(m_model.GetColumnIdAtVisibleIndex(1), COLUMN_B);
	EXPECT_EQ(m_model.GetColumnIdAtVisibleIndex(2), COLUMN_C);

	m_model.SetColumnVisible(COLUMN_B, false);
	EXPECT_EQ(m_model.GetColumnIdAtVisibleIndex(0), COLUMN_A);
	EXPECT_EQ(m_model.GetColumnIdAtVisibleIndex(1), COLUMN_C);
}

TEST_F(ListViewColumnModelTest, IsColumnVisible)
{
	EXPECT_TRUE(m_model.IsColumnVisible(COLUMN_A));
	EXPECT_TRUE(m_model.IsColumnVisible(COLUMN_B));
	EXPECT_TRUE(m_model.IsColumnVisible(COLUMN_C));

	m_model.SetColumnVisible(COLUMN_B, false);
	EXPECT_TRUE(m_model.IsColumnVisible(COLUMN_A));
	EXPECT_FALSE(m_model.IsColumnVisible(COLUMN_B));
	EXPECT_TRUE(m_model.IsColumnVisible(COLUMN_C));

	m_model.SetColumnVisible(COLUMN_C, false);
	EXPECT_TRUE(m_model.IsColumnVisible(COLUMN_A));
	EXPECT_FALSE(m_model.IsColumnVisible(COLUMN_B));
	EXPECT_FALSE(m_model.IsColumnVisible(COLUMN_C));
}

TEST_F(ListViewColumnModelTest, PrimaryColumnVisibility)
{
	// Column A is the primary column, so attempting to change its visibility should have no effect.
	m_model.SetColumnVisible(COLUMN_A, false);
	EXPECT_TRUE(m_model.IsColumnVisible(COLUMN_A));
}

TEST_F(ListViewColumnModelTest, GetPrimaryColumnId)
{
	EXPECT_EQ(m_model.GetPrimaryColumnId(), COLUMN_A);
}

TEST_F(ListViewColumnModelTest, GetColumnById)
{
	for (const auto &column : m_initialColumns)
	{
		EXPECT_EQ(m_model.GetColumnById(column.id), column);
	}
}

TEST_F(ListViewColumnModelTest, MoveColumn)
{
	m_model.MoveColumn(COLUMN_A, 2);
	EXPECT_THAT(GeneratorToVector(m_model.GetAllColumnIds()),
		ElementsAre(COLUMN_B, COLUMN_C, COLUMN_A));

	m_model.MoveColumn(COLUMN_B, 1);
	EXPECT_THAT(GeneratorToVector(m_model.GetAllColumnIds()),
		ElementsAre(COLUMN_C, COLUMN_B, COLUMN_A));
}

TEST_F(ListViewColumnModelTest, VisibilityChangedSignal)
{
	MockFunction<void(ListViewColumnId columnId, bool visible)> callback;
	m_model.columnVisibilityChangedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call(COLUMN_B, false));
	m_model.SetColumnVisible(COLUMN_B, false);

	EXPECT_CALL(callback, Call(COLUMN_B, true));
	m_model.SetColumnVisible(COLUMN_B, true);
}

TEST_F(ListViewColumnModelTest, ColumnMovedSignal)
{
	MockFunction<void(ListViewColumnId columnId, int newVisibleIndex)> callback;
	m_model.columnMovedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call(COLUMN_B, 2));
	m_model.MoveColumn(COLUMN_B, 2);
}
