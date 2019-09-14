// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Accelerator.h"
#include <boost/optional.hpp>

class AcceleratorUpdater
{
public:

	AcceleratorUpdater(HACCEL *acceleratorTable);
	~AcceleratorUpdater();

	void update(std::vector<ShortcutKey> shortcutKeys);

private:

	HACCEL *m_acceleratorTable;
};