// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ListViewColumn.h"
#include "ListViewColumnModel.h"
#include <vector>

class ListViewColumnModelFake : public ListViewColumnModel
{
public:
	static constexpr ListViewColumnId COLUMN_NAME{ 1 };
	static constexpr ListViewColumnId COLUMN_DATA_1{ 2 };
	static constexpr ListViewColumnId COLUMN_DATA_2{ 3 };

	ListViewColumnModelFake();

private:
	static std::vector<ListViewColumn> BuildColumnSet();
};
