// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AcceleratorUpdater.h"
#include "AcceleratorMappings.h"

AcceleratorUpdater::AcceleratorUpdater(HACCEL *acceleratorTable) :
	m_acceleratorTable(acceleratorTable)
{

}

AcceleratorUpdater::~AcceleratorUpdater()
{

}

void AcceleratorUpdater::update(std::vector<ShortcutKey> shortcutKeys)
{
	int numAccelerators = CopyAcceleratorTable(*m_acceleratorTable, nullptr, 0);

	std::vector<ACCEL> accelerators(numAccelerators);
	CopyAcceleratorTable(*m_acceleratorTable, &accelerators[0], static_cast<int>(accelerators.size()));

	for (const auto &shortcutKey : shortcutKeys)
	{
		accelerators.erase(std::remove_if(accelerators.begin(), accelerators.end(), [shortcutKey] (const ACCEL &accel) {
			return accel.cmd == shortcutKey.command;
		}), accelerators.end());

		for (const auto &key : shortcutKey.accelerators)
		{
			auto itr = std::find_if(accelerators.begin(), accelerators.end(), [key] (const ACCEL &accel) {
				return (accel.fVirt & ~FNOINVERT) == key.modifiers && accel.key == key.key;
			});

			if (itr != accelerators.end())
			{
				continue;
			}

			ACCEL newAccel;
			newAccel.fVirt = key.modifiers;
			newAccel.key = key.key;
			newAccel.cmd = static_cast<WORD>(shortcutKey.command);
			accelerators.push_back(newAccel);
		}
	}

	HACCEL newAcceleratorTable = CreateAcceleratorTable(&accelerators[0], static_cast<int>(accelerators.size()));

	if (newAcceleratorTable == nullptr)
	{
		return;
	}

	DestroyAcceleratorTable(*m_acceleratorTable);
	*m_acceleratorTable = newAcceleratorTable;
}