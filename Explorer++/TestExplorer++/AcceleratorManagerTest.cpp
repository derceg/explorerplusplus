// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "AcceleratorManager.h"
#include "AcceleratorHelper.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace testing;

bool operator==(const ACCEL &first, const ACCEL &second)
{
	return first.cmd == second.cmd && first.fVirt == second.fVirt && first.key == second.key;
}

class AcceleratorManagerTest : public Test
{
protected:
	AcceleratorManagerTest()
	{
		AddAccelerator(FVIRTKEY, VK_DELETE);
		AddAccelerator(FVIRTKEY, VK_F1);
		AddAccelerator(FVIRTKEY | FCONTROL, 'C');

		m_acceleratorManager =
			std::make_unique<AcceleratorManager>(AcceleratorItemsToTable(m_accelerators));
	}

	void AddAccelerator(BYTE flags, WORD command)
	{
		m_accelerators.push_back({ flags, command, m_commandIdCounter++ });
	}

	WORD m_commandIdCounter = 1;
	std::vector<ACCEL> m_accelerators;
	std::unique_ptr<AcceleratorManager> m_acceleratorManager;
};

TEST_F(AcceleratorManagerTest, GetAccelerators)
{
	for (const auto &accelerator : m_accelerators)
	{
		auto retrievedAccelerator = m_acceleratorManager->GetAcceleratorForCommand(accelerator.cmd);
		EXPECT_EQ(retrievedAccelerator, accelerator);
	}
}

TEST_F(AcceleratorManagerTest, GetNonExistentAccelerator)
{
	auto retrievedAccelerator = m_acceleratorManager->GetAcceleratorForCommand(m_commandIdCounter);
	EXPECT_EQ(retrievedAccelerator, std::nullopt);
}

TEST_F(AcceleratorManagerTest, SetAccelerators)
{
	auto originalAcceleratorTable = m_acceleratorManager->GetAcceleratorTable();

	AddAccelerator(FVIRTKEY | FCONTROL, 'X');
	AddAccelerator(FVIRTKEY | FCONTROL, 'V');

	m_acceleratorManager->SetAccelerators(m_accelerators);

	// As the accelerators were updated, the accelerator table should have been rebuilt.
	EXPECT_NE(m_acceleratorManager->GetAcceleratorTable(), originalAcceleratorTable);
	EXPECT_EQ(m_acceleratorManager->GetAccelerators(), m_accelerators);
}
