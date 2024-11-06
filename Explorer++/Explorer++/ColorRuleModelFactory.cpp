// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColorRuleModelFactory.h"
#include "ColorRule.h"
#include "ColorRuleModel.h"
#include "MainResource.h"
#include "ResourceHelper.h"

std::unique_ptr<ColorRuleModel> ColorRuleModelFactory::Create()
{
	auto colorRuleModel = std::make_unique<ColorRuleModel>();

	colorRuleModel->AddItem(std::make_unique<ColorRule>(
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_GENERAL_COLOR_RULE_COMPRESSED),
		L"", false, FILE_ATTRIBUTE_COMPRESSED, RGB(0, 116, 232)));

	colorRuleModel->AddItem(std::make_unique<ColorRule>(
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_GENERAL_COLOR_RULE_ENCRYPTED), L"",
		false, FILE_ATTRIBUTE_ENCRYPTED, RGB(0, 128, 0)));

	return colorRuleModel;
}
