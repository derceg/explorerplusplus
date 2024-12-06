// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DriveEnumeratorImpl.h"
#include <gtest/gtest.h>

TEST(DriveEnumeratorImplTest, GetDrives)
{
	DriveEnumeratorImpl driveEnumerator;
	auto drivesResult = driveEnumerator.GetDrives();
	ASSERT_TRUE(drivesResult);

	// Although the list of drives can vary, there should always be at least one drive returned (the
	// system drive).
	const auto &drives = drivesResult.value();
	EXPECT_GE(drives.size(), 1u);
}
