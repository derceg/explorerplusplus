// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>

class ColorRuleModel;

class ColorRuleModelFactory
{
public:
	static ColorRuleModelFactory *GetInstance();

	ColorRuleModel *GetColorRuleModel();

private:
	ColorRuleModelFactory() = default;
	~ColorRuleModelFactory();

	static inline ColorRuleModelFactory *m_staticInstance = nullptr;

	std::unique_ptr<ColorRuleModel> m_colorRuleModel;
};
