// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/AutoReset.h"
#include <gtest/gtest.h>

TEST(AutoResetTest, ResetValue)
{
	bool value = false;

	{
		AutoReset autoReset(&value, true);
		EXPECT_EQ(value, true);
	}

	EXPECT_EQ(value, false);
}
