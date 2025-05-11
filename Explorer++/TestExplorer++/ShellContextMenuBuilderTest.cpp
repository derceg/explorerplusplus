// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ShellContextMenuBuilder.h"
#include "ShellContextMenuDelegateFake.h"
#include "../Helper/Helper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellContextMenu.h"
#include "../Helper/ShellContextMenuIdGenerator.h"
#include "../Helper/ShellContextMenuIdRemapper.h"
#include "../Helper/WinRTBaseWrapper.h"
#include <gtest/gtest.h>
#include <unordered_map>

using namespace testing;

namespace
{

class ContextMenuFake : public winrt::implements<ContextMenuFake, IContextMenu, winrt::non_agile>
{
public:
	IFACEMETHODIMP QueryContextMenu(HMENU menu, UINT startIndex, UINT minId, UINT maxId, UINT flags)
	{
		UNREFERENCED_PARAMETER(menu);
		UNREFERENCED_PARAMETER(startIndex);
		UNREFERENCED_PARAMETER(minId);
		UNREFERENCED_PARAMETER(maxId);
		UNREFERENCED_PARAMETER(flags);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP InvokeCommand(CMINVOKECOMMANDINFO *commandInfo)
	{
		UNREFERENCED_PARAMETER(commandInfo);

		return E_NOTIMPL;
	}

	IFACEMETHODIMP GetCommandString(UINT_PTR cmd, UINT type, UINT *reserved, CHAR *name,
		UINT bufferLength)
	{
		UNREFERENCED_PARAMETER(reserved);

		if (type != GCS_VERBW)
		{
			return E_FAIL;
		}

		auto itr = m_idToVerbMap.find(CheckedNumericCast<UINT>(cmd));

		if (itr == m_idToVerbMap.end())
		{
			return E_FAIL;
		}

		return StringCchCopy(reinterpret_cast<wchar_t *>(name), bufferLength, itr->second.c_str());
	}

	void SetVerbForId(UINT id, const std::wstring &verb)
	{
		auto [itr, didInsert] = m_idToVerbMap.insert({ id, verb });
		ASSERT_TRUE(didInsert);
	}

private:
	std::unordered_map<UINT, std::wstring> m_idToVerbMap;
};

}

class ShellContextMenuBuilderTest : public Test
{
protected:
	ShellContextMenuBuilderTest() :
		m_idGenerator(1),
		m_idRemapper(&m_delegate, &m_idGenerator),
		m_menu(CreatePopupMenu()),
		m_contextMenu(winrt::make_self<ContextMenuFake>()),
		m_builder(m_menu.get(), m_contextMenu.get(), &m_idRemapper)
	{
	}

	ShellContextMenuIdGenerator m_idGenerator;
	ShellContextMenuDelegateFake m_delegate;
	ShellContextMenuIdRemapper m_idRemapper;
	wil::unique_hmenu m_menu;
	winrt::com_ptr<ContextMenuFake> m_contextMenu;
	ShellContextMenuBuilder m_builder;
};

TEST_F(ShellContextMenuBuilderTest, AddStringItem)
{
	UINT originalId = 100;
	m_builder.AddStringItem(originalId, L"Item", 0, true);

	ASSERT_EQ(GetMenuItemCount(m_menu.get()), 1);

	UINT updatedId = GetMenuItemID(m_menu.get(), 0);
	EXPECT_EQ(updatedId, m_idRemapper.GetUpdatedId(originalId));
}

TEST_F(ShellContextMenuBuilderTest, EnableItem)
{
	UINT originalId = 100;
	m_builder.AddStringItem(originalId, L"Item", 0, true);
	ASSERT_TRUE(
		MenuHelper::IsMenuItemEnabled(m_menu.get(), m_idRemapper.GetUpdatedId(originalId), false));

	m_builder.EnableItem(originalId, false);
	EXPECT_FALSE(
		MenuHelper::IsMenuItemEnabled(m_menu.get(), m_idRemapper.GetUpdatedId(originalId), false));
}

TEST_F(ShellContextMenuBuilderTest, AddSubMenuItem)
{
	const UINT startId = 1006;

	wil::unique_hmenu subMenu(CreatePopupMenu());
	MenuHelper::AddStringItem(subMenu.get(), startId, L"First");
	MenuHelper::AddStringItem(subMenu.get(), startId + 1, L"Second");
	MenuHelper::AddSeparator(subMenu.get());
	MenuHelper::AddStringItem(subMenu.get(), startId + 2, L"Third");

	m_builder.AddSubMenuItem(L"Sub menu", std::move(subMenu), 0, true);
	ASSERT_EQ(GetMenuItemCount(m_menu.get()), 1);

	HMENU insertedSubMenu = GetSubMenu(m_menu.get(), 0);
	ASSERT_NE(insertedSubMenu, nullptr);

	UINT updatedId1 = GetMenuItemID(insertedSubMenu, 0);
	UINT updatedId2 = GetMenuItemID(insertedSubMenu, 1);
	UINT updatedId3 = GetMenuItemID(insertedSubMenu, 3);

	EXPECT_EQ(updatedId1, m_idRemapper.GetUpdatedId(startId));
	EXPECT_EQ(updatedId2, m_idRemapper.GetUpdatedId(startId + 1));
	EXPECT_EQ(updatedId3, m_idRemapper.GetUpdatedId(startId + 2));
}

class ShellContextMenuBuilderRemoveTest : public ShellContextMenuBuilderTest
{
protected:
	void AddStringItem(UINT id, const std::wstring &text)
	{
		MenuHelper::AddStringItem(m_menu.get(), OffsetId(id), text);
	}

	UINT OffsetId(UINT id)
	{
		return ShellContextMenu::MIN_SHELL_MENU_ID + id;
	}
};

TEST_F(ShellContextMenuBuilderRemoveTest, RemoveShellItem)
{
	UINT idCounter = 0;
	UINT id1 = idCounter++;
	AddStringItem(id1, L"Item 1");

	UINT id2 = idCounter++;
	AddStringItem(id2, L"Item 2");

	std::wstring verb = L"action";
	m_contextMenu->SetVerbForId(id1, verb);

	// The verb was associated with item 1, so that item should be removed from the menu, leaving
	// only item 2.
	m_builder.RemoveShellItem(verb);
	ASSERT_EQ(GetMenuItemCount(m_menu.get()), 1);
	EXPECT_EQ(GetMenuItemID(m_menu.get(), 0), OffsetId(id2));
}
