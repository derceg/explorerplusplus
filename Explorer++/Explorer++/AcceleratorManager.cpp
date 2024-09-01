// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AcceleratorManager.h"
#include "AcceleratorHelper.h"

AcceleratorManager::AcceleratorManager(std::span<const ACCEL> accelerators)
{
	SetAccelerators(accelerators);
}

HACCEL AcceleratorManager::GetAcceleratorTable() const
{
	return m_acceleratorTable.get();
}

const std::vector<ACCEL> &AcceleratorManager::GetAccelerators() const
{
	return m_accelerators;
}

void AcceleratorManager::SetAccelerators(std::span<const ACCEL> updatedAccelerators)
{
	DCHECK(AreAcceleratorsValid(updatedAccelerators));

	m_acceleratorTable = AcceleratorItemsToTable(updatedAccelerators);
	m_accelerators = { updatedAccelerators.begin(), updatedAccelerators.end() };
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
