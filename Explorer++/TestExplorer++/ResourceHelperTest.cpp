// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ResourceHelper.h"
#include "TestResources.h"
#include <gtest/gtest.h>

TEST(MakeRTLDialogTemplate, Basic)
{
	auto dialogTemplateEx = MakeRTLDialogTemplate(GetModuleHandle(nullptr), IDD_BASIC_DIALOG);
	ASSERT_NE(dialogTemplateEx, nullptr);

	EXPECT_TRUE(WI_IsFlagSet(dialogTemplateEx->exStyle, WS_EX_LAYOUTRTL));
}
