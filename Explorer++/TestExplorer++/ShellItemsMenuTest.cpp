// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellItemsMenu.h"
#include "BrowserWindow.h"
#include "IconFetcherMock.h"
#include "../Helper/ShellHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class BrowserWindowMock : public BrowserWindow
{
public:
	// BrowserWindow
	MOCK_METHOD(BrowserCommandController *, GetCommandController, (), (override));
	MOCK_METHOD(BrowserPane *, GetActivePane, (), (const, override));
	MOCK_METHOD(void, FocusActiveTab, (), (override));

	// Navigator
	MOCK_METHOD(void, OpenItem,
		(const std::wstring &itemPath, OpenFolderDisposition openFolderDisposition), (override));
	MOCK_METHOD(void, OpenItem,
		(PCIDLIST_ABSOLUTE pidlItem, OpenFolderDisposition openFolderDisposition), (override));
};

class ShellItemsMenuTest : public Test
{
protected:
	enum class SelectionType
	{
		Click,
		MiddleClick
	};

	ShellItemsMenuTest() : m_pidls(GetPidls()), m_menu(m_pidls, &m_browserWindow, &m_iconFetcher)
	{
	}

	std::wstring GetNameForItem(size_t index) const
	{
		return L"Fake" + std::to_wstring(index);
	}

	void TestSelection(SelectionType selectionType)
	{
		for (size_t i = 0; i < m_pidls.size(); i++)
		{
			TestSelectionForItem(i, selectionType);
		}
	}

	std::vector<PidlAbsolute> m_pidls;
	BrowserWindowMock m_browserWindow;
	IconFetcherMock m_iconFetcher;
	ShellItemsMenu m_menu;

private:
	std::vector<PidlAbsolute> GetPidls() const
	{
		std::vector<PidlAbsolute> pidls;

		for (int i = 0; i < 3; i++)
		{
			auto path = L"C:\\" + GetNameForItem(i);
			AddPath(pidls, path);
		}

		return pidls;
	}

	void AddPath(std::vector<PidlAbsolute> &pidls, const std::wstring &path) const
	{
		unique_pidl_absolute pidl(SHSimpleIDListFromPath(path.c_str()));
		ASSERT_NE(pidl, nullptr);
		pidls.push_back(pidl.get());
	}

	void TestSelectionForItem(size_t index, SelectionType selectionType)
	{
		auto disposition = (selectionType == SelectionType::Click)
			? OpenFolderDisposition::CurrentTab
			: OpenFolderDisposition::NewTabDefault;

		EXPECT_CALL(m_browserWindow,
			OpenItem(Matcher<PCIDLIST_ABSOLUTE>(
						 Truly(std::bind_front(&ArePidlsEquivalent, m_pidls[index].Raw()))),
				disposition));

		auto menuView = m_menu.GetMenuViewForTesting();

		if (selectionType == SelectionType::Click)
		{
			m_menu.OnMenuItemSelected(menuView->GetItemIdForTesting(static_cast<int>(index)), false,
				false);
		}
		else
		{
			m_menu.OnMenuItemMiddleClicked(menuView->GetItemIdForTesting(static_cast<int>(index)),
				false, false);
		}
	}
};

TEST_F(ShellItemsMenuTest, CheckItems)
{
	auto menuView = m_menu.GetMenuViewForTesting();

	EXPECT_EQ(menuView->GetItemCountForTesting(), m_pidls.size());

	for (size_t i = 0; i < m_pidls.size(); i++)
	{
		EXPECT_EQ(
			menuView->GetItemTextForTesting(menuView->GetItemIdForTesting(static_cast<int>(i))),
			GetNameForItem(i));
	}
}

TEST_F(ShellItemsMenuTest, Selection)
{
	TestSelection(SelectionType::Click);
	TestSelection(SelectionType::MiddleClick);
}
