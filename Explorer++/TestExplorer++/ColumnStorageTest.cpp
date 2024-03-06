// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ColumnStorage.h"
#include "DefaultColumns.h"
#include <gtest/gtest.h>

TEST(ValidateSingleColumnSetTest, DefaultColumnsOnly)
{
	// This set contains the default columns, just in a different order. The set shouldn't be
	// modified by ValidateSingleColumnSet().
	// clang-format off
	std::vector<Column_t> columns = {
		{ ColumnType::NetworkAdaptorStatus, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Owner, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH }
	};
	// clang-format on

	ValidateSingleColumnSet(ColumnValidationType::NetworkConnections, columns);

	// clang-format off
	std::vector<Column_t> expectedColumns = {
		{ ColumnType::NetworkAdaptorStatus, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Owner, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH }
	};
	// clang-format on

	EXPECT_EQ(columns, expectedColumns);
}

TEST(ValidateSingleColumnSetTest, MissingColumns)
{
	// This set is missing columns that appear in the default column set, so they should be added
	// back by ValidateSingleColumnSet().
	// clang-format off
	std::vector<Column_t> columns = {
		{ ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Owner, TRUE, DEFAULT_COLUMN_WIDTH }
	};
	// clang-format on

	ValidateSingleColumnSet(ColumnValidationType::NetworkConnections, columns);

	// clang-format off
	std::vector<Column_t> expectedColumns = {
		{ ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Owner, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::NetworkAdaptorStatus, TRUE, DEFAULT_COLUMN_WIDTH }
	};
	// clang-format on

	EXPECT_EQ(columns, expectedColumns);
}

TEST(ValidateSingleColumnSetTest, UnknownColumns)
{
	// This set contains columns that don't appear in the default column set, so they should be
	// removed by ValidateSingleColumnSet().
	// clang-format off
	std::vector<Column_t> columns = {
		{ ColumnType::PrinterStatus, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::NetworkAdaptorStatus, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::OriginalLocation, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Owner, TRUE, DEFAULT_COLUMN_WIDTH }
	};
	// clang-format on

	ValidateSingleColumnSet(ColumnValidationType::NetworkConnections, columns);

	// clang-format off
	std::vector<Column_t> expectedColumns = {
		{ ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::NetworkAdaptorStatus, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Owner, TRUE, DEFAULT_COLUMN_WIDTH }
	};
	// clang-format on

	EXPECT_EQ(columns, expectedColumns);
}
