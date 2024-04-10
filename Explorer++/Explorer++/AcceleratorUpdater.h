// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Accelerator.h"

class AcceleratorManager;

class AcceleratorUpdater
{
public:
	AcceleratorUpdater(AcceleratorManager *acceleratorManager);

	void update(const std::vector<ShortcutKey> &shortcutKeys);

private:
	AcceleratorManager *const m_acceleratorManager;
};
