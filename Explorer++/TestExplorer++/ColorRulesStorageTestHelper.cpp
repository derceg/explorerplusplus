// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ColorRulesStorageTestHelper.h"
#include "ColorRule.h"
#include "ColorRuleModel.h"

bool operator==(const ColorRule &first, const ColorRule &second)
{
	return first.GetDescription() == second.GetDescription()
		&& first.GetFilterPattern() == second.GetFilterPattern()
		&& first.GetFilterPatternCaseInsensitive() == second.GetFilterPatternCaseInsensitive()
		&& first.GetFilterAttributes() == second.GetFilterAttributes()
		&& first.GetColor() == second.GetColor();
}

void BuildLoadSaveReferenceModel(ColorRuleModel *model)
{
	model->AddItem(std::make_unique<ColorRule>(L"Compressed files", L"", false,
		FILE_ATTRIBUTE_COMPRESSED, RGB(0, 0, 255)));
	model->AddItem(std::make_unique<ColorRule>(L"Encrypted files", L"", false,
		FILE_ATTRIBUTE_ENCRYPTED, RGB(0, 128, 0)));
	model->AddItem(
		std::make_unique<ColorRule>(L"Text files", L"*.txt", true, 0, RGB(192, 192, 192)));
	model->AddItem(
		std::make_unique<ColorRule>(L"Log files", L"*.log", false, 0, RGB(128, 128, 255)));
}
