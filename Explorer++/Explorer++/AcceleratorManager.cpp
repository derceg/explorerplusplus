// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AcceleratorManager.h"
#include "AcceleratorHelper.h"
#include <boost/range/join.hpp>

AcceleratorManager::AcceleratorManager(std::span<const ACCEL> accelerators,
	std::span<const ACCEL> nonAcceleratorShortcuts) :
	m_nonAcceleratorShortcuts(nonAcceleratorShortcuts.begin(), nonAcceleratorShortcuts.end())
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
	auto combinedItems = boost::range::join(m_accelerators, m_nonAcceleratorShortcuts);
	auto itr = std::ranges::find_if(combinedItems,
		[command](const ACCEL &accel) { return accel.cmd == command; });

	if (itr == combinedItems.end())
	{
		return std::nullopt;
	}

	return *itr;
}
