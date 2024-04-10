// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AcceleratorUpdater.h"
#include "AcceleratorManager.h"

AcceleratorUpdater::AcceleratorUpdater(AcceleratorManager *acceleratorManager) :
	m_acceleratorManager(acceleratorManager)
{
}

void AcceleratorUpdater::update(const std::vector<ShortcutKey> &shortcutKeys)
{
	auto accelerators = m_acceleratorManager->GetAccelerators();

	for (const auto &shortcutKey : shortcutKeys)
	{
		accelerators.erase(std::remove_if(accelerators.begin(), accelerators.end(),
							   [shortcutKey](const ACCEL &accel)
							   { return accel.cmd == shortcutKey.command; }),
			accelerators.end());

		for (const auto &key : shortcutKey.accelerators)
		{
			auto itr = std::find_if(accelerators.begin(), accelerators.end(),
				[key](const ACCEL &accel)
				{ return (accel.fVirt & ~FNOINVERT) == key.modifiers && accel.key == key.key; });

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

	m_acceleratorManager->SetAccelerators(accelerators);
}
