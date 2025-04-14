// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowser/ColumnHelper.h"
#include "ResourceLoaderFake.h"
#include <gtest/gtest.h>

TEST(ColumnHelperTest, CheckEveryColumnHasAName)
{
	ResourceLoaderFake resourceLoader;

	// Each column should have a name. `GetColumnName` will trigger a CHECK failure if a particular
	// column isn't handled, so this test will fail in that case.
	for (auto columnType : ColumnType::_values())
	{
		GetColumnName(&resourceLoader, columnType);
	}
}
