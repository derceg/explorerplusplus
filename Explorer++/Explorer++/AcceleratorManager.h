// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <optional>
#include <vector>

class AcceleratorManager
{
public:
	// Initializes the accelerators from the provided accelerator table.
	AcceleratorManager(wil::unique_haccel acceleratorTable);

	HACCEL GetAcceleratorTable() const;
	const std::vector<ACCEL> &GetAccelerators() const;
	void SetAccelerators(const std::vector<ACCEL> &updatedAccelerators);
	std::optional<ACCEL> GetAcceleratorForCommand(WORD command) const;

private:
	wil::unique_haccel m_acceleratorTable;

	// Matches the set of items that appear in the accelerator table. Technically redundant, but
	// storing this means the accelerators don't have to be copied every time the list of
	// accelerator items is needed.
	std::vector<ACCEL> m_accelerators;
};
