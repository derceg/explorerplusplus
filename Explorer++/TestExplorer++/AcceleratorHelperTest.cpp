// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "AcceleratorHelper.h"
#include <gtest/gtest.h>

TEST(BuildAcceleratorStringTest, BuildString)
{
	ACCEL accelerator = {};
	accelerator.fVirt = FVIRTKEY;
	accelerator.key = VK_F1;
	auto acceleratorText = BuildAcceleratorString(accelerator);
	EXPECT_EQ(acceleratorText, L"F1");

	accelerator = {};
	accelerator.fVirt = FVIRTKEY | FCONTROL;
	accelerator.key = 'A';
	acceleratorText = BuildAcceleratorString(accelerator);
	EXPECT_EQ(acceleratorText, L"Ctrl+A");

	accelerator = {};
	accelerator.fVirt = FVIRTKEY | FCONTROL | FSHIFT;
	accelerator.key = 'P';
	acceleratorText = BuildAcceleratorString(accelerator);
	EXPECT_EQ(acceleratorText, L"Ctrl+Shift+P");
}
