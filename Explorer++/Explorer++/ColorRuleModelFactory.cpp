// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColorRuleModelFactory.h"
#include "ColorRuleModel.h"

ColorRuleModelFactory::~ColorRuleModelFactory() = default;

ColorRuleModelFactory *ColorRuleModelFactory::GetInstance()
{
	if (!m_staticInstance)
	{
		m_staticInstance = new ColorRuleModelFactory();
	}

	return m_staticInstance;
}

ColorRuleModel *ColorRuleModelFactory::GetColorRuleModel()
{
	if (!m_colorRuleModel)
	{
		m_colorRuleModel = std::make_unique<ColorRuleModel>();
	}

	return m_colorRuleModel.get();
}
