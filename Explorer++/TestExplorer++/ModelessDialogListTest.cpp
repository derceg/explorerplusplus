// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ModelessDialogList.h"
#include "GeneratorTestHelper.h"
#include "../Helper/WindowHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class ModelessDialogListTest : public Test
{
protected:
	ModelessDialogList m_modelessDialogList;
};

TEST_F(ModelessDialogListTest, AddRemoveDialog)
{
	std::wstring dialogId1 = L"dialog1";
	auto dialog1 = CreateMessageOnlyWindow();
	m_modelessDialogList.AddDialog(dialogId1, dialog1.get());
	EXPECT_THAT(GeneratorToVector(m_modelessDialogList.GetList()),
		UnorderedElementsAre(dialog1.get()));

	std::wstring dialogId2 = L"dialog2";
	auto dialog2 = CreateMessageOnlyWindow();
	m_modelessDialogList.AddDialog(dialogId2, dialog2.get());
	EXPECT_THAT(GeneratorToVector(m_modelessDialogList.GetList()),
		UnorderedElementsAre(dialog1.get(), dialog2.get()));

	m_modelessDialogList.RemoveDialog(dialogId1);
	EXPECT_THAT(GeneratorToVector(m_modelessDialogList.GetList()),
		UnorderedElementsAre(dialog2.get()));

	m_modelessDialogList.RemoveDialog(dialogId2);
	EXPECT_THAT(GeneratorToVector(m_modelessDialogList.GetList()), IsEmpty());
}

TEST_F(ModelessDialogListTest, MaybeGetDialogById)
{
	std::wstring dialogId = L"dialog";
	auto dialog = CreateMessageOnlyWindow();
	m_modelessDialogList.AddDialog(dialogId, dialog.get());
	EXPECT_EQ(m_modelessDialogList.MaybeGetDialogById(dialogId), dialog.get());

	EXPECT_EQ(m_modelessDialogList.MaybeGetDialogById(L"NonExistentDialog"), nullptr);
}
