// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "AcceleratorHelper.h"
#include "AcceleratorTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(AcceleratorHelperTest, BuildAcceleratorString)
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

TEST(AcceleratorHelperTest, AcceleratorTableConversion)
{
	WORD commandIdCounter = 1;
	const ACCEL accelerators[] = { { FVIRTKEY | FCONTROL, 'A', commandIdCounter++ },
		{ FVIRTKEY | FCONTROL | FSHIFT, VK_F6, commandIdCounter++ },
		{ FVIRTKEY | FALT, 'K', commandIdCounter++ }, { FVIRTKEY, VK_F2, commandIdCounter++ } };
	auto table = AcceleratorItemsToTable(accelerators);
	auto outputAccelerators = TableToAcceleratorItems(table.get());
	EXPECT_THAT(outputAccelerators, ElementsAreArray(accelerators));
}
