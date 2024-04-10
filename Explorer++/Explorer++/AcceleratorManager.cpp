// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AcceleratorManager.h"
#include "AcceleratorHelper.h"

AcceleratorManager::AcceleratorManager(wil::unique_haccel acceleratorTable)
{
	m_acceleratorTable = std::move(acceleratorTable);
	m_accelerators = TableToAcceleratorItems(m_acceleratorTable.get());
}

HACCEL AcceleratorManager::GetAcceleratorTable() const
{
	return m_acceleratorTable.get();
}

const std::vector<ACCEL> &AcceleratorManager::GetAccelerators() const
{
	return m_accelerators;
}

void AcceleratorManager::SetAccelerators(const std::vector<ACCEL> &updatedAccelerators)
{
	m_acceleratorTable = AcceleratorItemsToTable(updatedAccelerators);
	m_accelerators = updatedAccelerators;
}

std::optional<ACCEL> AcceleratorManager::GetAcceleratorForCommand(WORD command) const
{
	auto itr = std::find_if(m_accelerators.begin(), m_accelerators.end(),
		[command](const ACCEL &accel) { return accel.cmd == command; });

	if (itr == m_accelerators.end())
	{
		return std::nullopt;
	}

	return *itr;
}
