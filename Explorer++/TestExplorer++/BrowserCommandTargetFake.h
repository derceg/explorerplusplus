// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BrowserCommandTarget.h"

class BrowserCommandTargetFake : public BrowserCommandTarget
{
public:
	bool IsCommandEnabled(int command) const override;
	void ExecuteCommand(int command) override;
};
