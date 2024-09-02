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

	// This should generate the string "Ctrl++" (and not something like "Ctrl+Num +").
	accelerator = {};
	accelerator.fVirt = FVIRTKEY | FCONTROL;
	accelerator.key = VK_ADD;
	acceleratorText = BuildAcceleratorString(accelerator);
	EXPECT_EQ(acceleratorText, L"Ctrl++");

	// And this should be "Ctrl+-" (not "Ctrl+Num -").
	accelerator = {};
	accelerator.fVirt = FVIRTKEY | FCONTROL;
	accelerator.key = VK_SUBTRACT;
	acceleratorText = BuildAcceleratorString(accelerator);
	EXPECT_EQ(acceleratorText, L"Ctrl+-");
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

TEST(AcceleratorHelperTest, Validity)
{
	WORD commandIdCounter = 1;
	EXPECT_FALSE(DoAcceleratorsContainCtrlAlt({ { { FVIRTKEY | FCONTROL, 'T', commandIdCounter++ },
		{ FVIRTKEY | FCONTROL | FSHIFT, 'N', commandIdCounter++ } } }));
	EXPECT_TRUE(DoAcceleratorsContainCtrlAlt({ { { FVIRTKEY, VK_F1, commandIdCounter++ },
		{ FVIRTKEY | FCONTROL | FALT, 'W', commandIdCounter++ } } }));

	EXPECT_FALSE(DoAcceleratorsContainDuplicates(
		{ { { FVIRTKEY | FCONTROL | FSHIFT, 'P', commandIdCounter++ },
			{ FVIRTKEY, VK_F3, commandIdCounter++ } } }));
	EXPECT_TRUE(
		DoAcceleratorsContainDuplicates({ { { FVIRTKEY | FCONTROL, 'A', commandIdCounter++ },
			{ FVIRTKEY | FCONTROL, 'A', commandIdCounter++ } } }));
}
