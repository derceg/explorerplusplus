// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellContextMenu.h"
#include "AcceleratorManager.h"
#include "MenuTestHost.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WinRTBaseWrapper.h"
#include <gtest/gtest.h>
#include <wil/com.h>
#include <unordered_set>

using namespace testing;

namespace
{

class ContextMenuFake : public winrt::implements<ContextMenuFake, IContextMenu, winrt::non_agile>
{
public:
	struct Item
	{
		std::wstring text;
		std::wstring verb;
		std::wstring helpText;
	};

	static inline const Item ITEMS[] = {
		{ L"Open", L"open", L"Opens an item" },
		{ L"Copy", L"copy", L"Copies an item" },
		{ L"Paste", L"paste", L"Pastes an item" },
	};

	IFACEMETHODIMP QueryContextMenu(HMENU menu, UINT startIndex, UINT minId, UINT maxId, UINT flags)
	{
		UNREFERENCED_PARAMETER(flags);

		UINT itemsAdded = 0;

		for (UINT i = 0; i < std::size(ContextMenuFake::ITEMS); i++)
		{
			UINT id = minId + i;

			if (id > maxId)
			{
				break;
			}

			const auto &item = ContextMenuFake::ITEMS[i];
			MenuHelper::AddStringItem(menu, id, item.text, startIndex + i,
				MenuHelper::ItemLookup::ByPosition);
			itemsAdded++;
		}

		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, itemsAdded);
	}

	IFACEMETHODIMP GetCommandString(UINT_PTR cmd, UINT type, UINT *reserved, CHAR *name,
		UINT bufferLength)
	{
		UNREFERENCED_PARAMETER(reserved);

		if (cmd >= std::size(ITEMS))
		{
			return E_INVALIDARG;
		}

		const auto &item = ITEMS[cmd];

		switch (type)
		{
		case GCS_VERBW:
			return StringCchCopy(reinterpret_cast<wchar_t *>(name), bufferLength,
				item.verb.c_str());

		case GCS_HELPTEXTW:
			return StringCchCopy(reinterpret_cast<wchar_t *>(name), bufferLength,
				item.helpText.c_str());
		}

		return E_INVALIDARG;
	}

	IFACEMETHODIMP InvokeCommand(CMINVOKECOMMANDINFO *commandInfo)
	{
		if (!IS_INTRESOURCE(commandInfo->lpVerb))
		{
			return E_INVALIDARG;
		}

		auto offset = static_cast<UINT>(reinterpret_cast<UINT_PTR>(commandInfo->lpVerb));

		if (offset >= std::size(ITEMS))
		{
			return E_INVALIDARG;
		}

		m_invokedCommandOffsets.push_back(offset);

		return S_OK;
	}

	const std::vector<UINT> &GetInvokedCommandOffsets() const
	{
		return m_invokedCommandOffsets;
	}

private:
	std::vector<UINT> m_invokedCommandOffsets;
};

class TestShellContextMenu : public ShellContextMenu
{
public:
	TestShellContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		wil::com_ptr_nothrow<IContextMenu> externalContextMenu) :
		ShellContextMenu(menuView, acceleratorManager, PidlAbsolute(), nullptr),
		m_externalContextMenu(externalContextMenu)
	{
		AppendShellItems(nullptr, CMF_NORMAL);
	}

	using ShellContextMenu::MaybeGetShellItemIndex;
	using ShellContextMenu::RemoveShellItems;

	void SetVerbsToHandleInternally(const std::unordered_set<std::wstring> &verbs)
	{
		m_verbsToHandleInternally = verbs;
	}

private:
	wil::com_ptr_nothrow<IContextMenu> MaybeGetShellContextMenu(HWND hwnd) const override
	{
		UNREFERENCED_PARAMETER(hwnd);

		return m_externalContextMenu;
	}

	bool MaybeHandleShellMenuItem(const std::wstring &verb) override
	{
		return m_verbsToHandleInternally.contains(verb);
	}

	wil::com_ptr_nothrow<IContextMenu> m_externalContextMenu;
	std::unordered_set<std::wstring> m_verbsToHandleInternally;
};

}

class ShellContextMenuTest : public Test
{
protected:
	ShellContextMenuTest() :
		m_externalContextMenu(winrt::make_self<ContextMenuFake>()),
		m_menu(m_menuHost.GetView(), &m_acceleratorManager, m_externalContextMenu.get())
	{
	}

	AcceleratorManager m_acceleratorManager;

	MenuTestHost m_menuHost;
	winrt::com_ptr<ContextMenuFake> m_externalContextMenu;
	TestShellContextMenu m_menu;
};

TEST_F(ShellContextMenuTest, MaybeGetShellItemIndex)
{
	for (UINT i = 0; i < std::size(ContextMenuFake::ITEMS); i++)
	{
		const auto &item = ContextMenuFake::ITEMS[i];
		EXPECT_EQ(m_menu.MaybeGetShellItemIndex(item.verb), i);
	}

	EXPECT_EQ(m_menu.MaybeGetShellItemIndex(L"non-existent"), std::nullopt);
}

TEST_F(ShellContextMenuTest, RemoveShellItems)
{
	m_menu.RemoveShellItems({ L"open", L"paste" });

	// The two items specified should have been removed, leaving just the copy item.
	auto *menuView = m_menuHost.GetView();
	ASSERT_EQ(menuView->GetNumItems(), 1);
	EXPECT_EQ(menuView->GetItemIdForTesting(0), MenuView::EXTERNAL_ID_MIN + 1);
}

TEST_F(ShellContextMenuTest, HelpText)
{
	auto *menuView = m_menuHost.GetView();

	for (UINT i = 0; i < std::size(ContextMenuFake::ITEMS); i++)
	{
		const auto &item = ContextMenuFake::ITEMS[i];
		EXPECT_EQ(menuView->GetItemHelpText(MenuView::EXTERNAL_ID_MIN + i), item.helpText);
	}
}

TEST_F(ShellContextMenuTest, Selection)
{
	auto *menuView = m_menuHost.GetView();

	menuView->SelectItem(MenuView::EXTERNAL_ID_MIN + 2, false, false);
	EXPECT_THAT(m_externalContextMenu->GetInvokedCommandOffsets(), ElementsAre(2));

	menuView->SelectItem(MenuView::EXTERNAL_ID_MIN + 0, false, false);
	EXPECT_THAT(m_externalContextMenu->GetInvokedCommandOffsets(), ElementsAre(2, 0));

	menuView->SelectItem(MenuView::EXTERNAL_ID_MIN + 1, false, false);
	EXPECT_THAT(m_externalContextMenu->GetInvokedCommandOffsets(), ElementsAre(2, 0, 1));
}

TEST_F(ShellContextMenuTest, SelectionWithInternalHandling)
{
	m_menu.SetVerbsToHandleInternally({ L"open" });

	auto *menuView = m_menuHost.GetView();

	// The "open" item is handled internally, so selecting it shouldn't result in a call to
	// InvokeCommand().
	menuView->SelectItem(MenuView::EXTERNAL_ID_MIN + 0, false, false);
	EXPECT_THAT(m_externalContextMenu->GetInvokedCommandOffsets(), IsEmpty());

	// Selecting the other two commands should work normally, though.
	menuView->SelectItem(MenuView::EXTERNAL_ID_MIN + 1, false, false);
	EXPECT_THAT(m_externalContextMenu->GetInvokedCommandOffsets(), ElementsAre(1));

	menuView->SelectItem(MenuView::EXTERNAL_ID_MIN + 2, false, false);
	EXPECT_THAT(m_externalContextMenu->GetInvokedCommandOffsets(), ElementsAre(1, 2));
}
