// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "AcceleratorManager.h"
#include "AcceleratorTestHelper.h"
#include <gtest/gtest.h>
#include <vector>

using namespace testing;

class AcceleratorManagerTest : public Test
{
protected:
	AcceleratorManagerTest() :
		m_accelerators({ { FVIRTKEY, VK_DELETE, m_commandIdCounter++ },
			{ FVIRTKEY, VK_F1, m_commandIdCounter++ },
			{ FVIRTKEY | FCONTROL, 'C', m_commandIdCounter++ } }),
		m_acceleratorManager(m_accelerators)
	{
	}

	WORD m_commandIdCounter = 1;
	std::vector<ACCEL> m_accelerators;
	AcceleratorManager m_acceleratorManager;
};

TEST_F(AcceleratorManagerTest, GetAccelerators)
{
	for (const auto &accelerator : m_accelerators)
	{
		auto retrievedAccelerator = m_acceleratorManager.GetAcceleratorForCommand(accelerator.cmd);
		EXPECT_EQ(retrievedAccelerator, accelerator);
	}
}

TEST_F(AcceleratorManagerTest, GetNonExistentAccelerator)
{
	auto retrievedAccelerator = m_acceleratorManager.GetAcceleratorForCommand(m_commandIdCounter);
	EXPECT_EQ(retrievedAccelerator, std::nullopt);
}

TEST_F(AcceleratorManagerTest, SetAccelerators)
{
	auto originalAcceleratorTable = m_acceleratorManager.GetAcceleratorTable();

	m_accelerators.push_back({ FVIRTKEY | FCONTROL, 'X', m_commandIdCounter++ });
	m_accelerators.push_back({ FVIRTKEY | FCONTROL, 'V', m_commandIdCounter++ });

	m_acceleratorManager.SetAccelerators(m_accelerators);

	// As the accelerators were updated, the accelerator table should have been rebuilt.
	EXPECT_NE(m_acceleratorManager.GetAcceleratorTable(), originalAcceleratorTable);
	EXPECT_EQ(m_acceleratorManager.GetAccelerators(), m_accelerators);
}
