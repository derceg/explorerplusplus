// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>

class ColorRuleModel;

class ColorRuleModelFactory
{
public:
	// Creates a color rule model, initialized with the default set of color rules.
	static std::unique_ptr<ColorRuleModel> Create();
};
