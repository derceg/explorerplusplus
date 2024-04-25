// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "DragDropHelper.h"
#include "../Helper/DragDropHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class PreferredDropEffectTestSuite : public TestWithParam<DWORD>
{
};

TEST_P(PreferredDropEffectTestSuite, PreferredDropEffect)
{
	wil::com_ptr_nothrow<IDataObject> dataObject;
	CreateShellDataObject(L"C:\\fake", ShellItemType::File, dataObject);

	DWORD assignedEffect = GetParam();
	EXPECT_HRESULT_SUCCEEDED(SetPreferredDropEffect(dataObject.get(), assignedEffect));

	DWORD retrievedEffect = DROPEFFECT_NONE;
	EXPECT_HRESULT_SUCCEEDED(GetPreferredDropEffect(dataObject.get(), retrievedEffect));

	EXPECT_EQ(retrievedEffect, assignedEffect);
}

INSTANTIATE_TEST_SUITE_P(CopyAndMoveEffects, PreferredDropEffectTestSuite,
	Values(DROPEFFECT_COPY, DROPEFFECT_MOVE));
