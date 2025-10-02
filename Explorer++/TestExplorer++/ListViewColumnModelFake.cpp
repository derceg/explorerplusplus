// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ListViewColumnModelFake.h"

ListViewColumnModelFake::ListViewColumnModelFake() :
	ListViewColumnModel(BuildColumnSet(), COLUMN_NAME)
{
}

std::vector<ListViewColumn> ListViewColumnModelFake::BuildColumnSet()
{
	std::vector<ListViewColumn> columns;
	columns.emplace_back(COLUMN_NAME, 1, 100, true);
	columns.emplace_back(COLUMN_DATA_1, 1, 100, true);
	columns.emplace_back(COLUMN_DATA_2, 1, 100, true);
	return columns;
}
