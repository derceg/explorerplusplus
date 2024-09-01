// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <span>

class AcceleratorManager;

void UpdateMenuAcceleratorStrings(HMENU menu, const AcceleratorManager *acceleratorManager);
std::wstring BuildAcceleratorString(const ACCEL &accelerator);

std::vector<ACCEL> TableToAcceleratorItems(HACCEL acceleratorTable);
wil::unique_haccel AcceleratorItemsToTable(std::span<const ACCEL> accelerators);

// As per https://devblogs.microsoft.com/oldnewthing/20040329-00/?p=40003, Ctrl+Alt shouldn't be
// used as a shortcut modifier, since it acts as an alternate shift key on some keyboard layouts.
constexpr bool DoAcceleratorsContainCtrlAlt(std::span<const ACCEL> accelerators)
{
	auto itr = std::find_if(accelerators.begin(), accelerators.end(),
		[](const auto &accel) { return WI_AreAllFlagsSet(accel.fVirt, FCONTROL | FALT); });
	return itr != accelerators.end();
}

// Checks whether there are any duplicate key entries within the specified set of accelerators.
// (i.e. two or more entries that have the same key combination). Duplicate entries are invalid,
// since only the first will actually be used.
constexpr bool DoAcceleratorsContainDuplicates(std::span<const ACCEL> accelerators)
{
	for (const auto &accelerator : accelerators)
	{
		auto numMatches = std::count_if(accelerators.begin(), accelerators.end(),
			[&accelerator](const auto &currentAccelerator)
			{
				return currentAccelerator.fVirt == accelerator.fVirt
					&& currentAccelerator.key == accelerator.key;
			});

		if (numMatches > 1)
		{
			return true;
		}
	}

	return false;
}

constexpr bool AreAcceleratorsValid(std::span<const ACCEL> accelerators)
{
	return !DoAcceleratorsContainCtrlAlt(accelerators)
		&& !DoAcceleratorsContainDuplicates(accelerators);
}
