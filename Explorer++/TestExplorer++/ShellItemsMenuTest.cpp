// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellItemsMenu.h"
#include "BrowserWindowMock.h"
#include "IconFetcher.h"
#include "PopupMenuView.h"
#include "../Helper/ShellHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace
{

void AddPathToCollection(std::vector<PidlAbsolute> &pidls, const std::wstring &path)
{
	unique_pidl_absolute pidl(SHSimpleIDListFromPath(path.c_str()));
	ASSERT_NE(pidl, nullptr);
	pidls.push_back(pidl.get());
}

std::wstring GetNameForItem(size_t index)
{
	return L"Fake" + std::to_wstring(index);
}

std::vector<PidlAbsolute> BuildPidlCollection(int size)
{
	std::vector<PidlAbsolute> pidls;

	for (int i = 0; i < size; i++)
	{
		auto path = L"C:\\" + GetNameForItem(i);
		AddPathToCollection(pidls, path);
	}

	return pidls;
}

class IconFetcherFake : public IconFetcher
{
public:
	// IconFetcher
	void QueueIconTask(std::wstring_view path, Callback callback) override
	{
		UNREFERENCED_PARAMETER(path);

		m_resultCallbacks.push_back(callback);
	}

	void QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback) override
	{
		UNREFERENCED_PARAMETER(pidl);

		m_resultCallbacks.push_back(callback);
	}

	void ClearQueue() override
	{
	}

	int GetCachedIconIndexOrDefault(const std::wstring &itemPath,
		DefaultIconType defaultIconType) const override
	{
		UNREFERENCED_PARAMETER(itemPath);
		UNREFERENCED_PARAMETER(defaultIconType);

		return 0;
	}

	std::optional<int> GetCachedIconIndex(const std::wstring &itemPath) const override
	{
		UNREFERENCED_PARAMETER(itemPath);

		return std::nullopt;
	}

	// IconFetcherFake custom methods
	void TriggerPendingResultCallbacks()
	{
		for (const auto &callback : m_resultCallbacks)
		{
			callback(0);
		}

		m_resultCallbacks.clear();
	}

private:
	std::vector<Callback> m_resultCallbacks;
};

}

class ShellItemsMenuTest : public Test
{
protected:
	std::unique_ptr<ShellItemsMenu> BuildMenu(MenuView *menuView,
		const std::vector<PidlAbsolute> &pidls)
	{
		return std::make_unique<ShellItemsMenu>(menuView, pidls, &m_browserWindow, &m_iconFetcher);
	}

	BrowserWindowMock m_browserWindow;
	IconFetcherFake m_iconFetcher;
};

TEST_F(ShellItemsMenuTest, CheckItems)
{
	PopupMenuView popupMenu;
	auto pidls = BuildPidlCollection(3);
	auto menu = BuildMenu(&popupMenu, pidls);

	ASSERT_EQ(static_cast<size_t>(popupMenu.GetItemCountForTesting()), pidls.size());

	for (size_t i = 0; i < pidls.size(); i++)
	{
		EXPECT_EQ(
			popupMenu.GetItemTextForTesting(popupMenu.GetItemIdForTesting(static_cast<int>(i))),
			GetNameForItem(i));
	}
}

TEST_F(ShellItemsMenuTest, IconRetrievalAfterMenuDestroyed)
{
	PopupMenuView popupMenu;
	auto pidls = BuildPidlCollection(3);
	auto menu = BuildMenu(&popupMenu, pidls);

	menu.reset();

	// If one or more icons are retrieved after the menu has been closed and destroyed, the menu
	// can't be updated, but that should still be a safe operation.
	m_iconFetcher.TriggerPendingResultCallbacks();
}

class ShellItemsMenuSelectionTest : public Test
{
protected:
	enum class SelectionType
	{
		Click,
		MiddleClick
	};

	ShellItemsMenuSelectionTest() :
		m_pidls(BuildPidlCollection(3)),
		m_menu(&m_popupMenu, m_pidls, &m_browserWindow, &m_iconFetcher)
	{
	}

	void TestSelection(SelectionType selectionType)
	{
		for (size_t i = 0; i < m_pidls.size(); i++)
		{
			TestSelectionForItem(i, selectionType);
		}
	}

private:
	void TestSelectionForItem(size_t index, SelectionType selectionType)
	{
		auto disposition = (selectionType == SelectionType::Click)
			? OpenFolderDisposition::CurrentTab
			: OpenFolderDisposition::NewTabDefault;

		EXPECT_CALL(m_browserWindow,
			OpenItem(Matcher<PCIDLIST_ABSOLUTE>(
						 Truly(std::bind_front(&ArePidlsEquivalent, m_pidls[index].Raw()))),
				disposition));

		auto id = m_popupMenu.GetItemIdForTesting(static_cast<int>(index));

		if (selectionType == SelectionType::Click)
		{
			m_popupMenu.SelectItem(id, false, false);
		}
		else
		{
			m_popupMenu.MiddleClickItem(id, false, false);
		}
	}

	PopupMenuView m_popupMenu;
	std::vector<PidlAbsolute> m_pidls;
	BrowserWindowMock m_browserWindow;
	IconFetcherFake m_iconFetcher;
	ShellItemsMenu m_menu;
};

TEST_F(ShellItemsMenuSelectionTest, Selection)
{
	TestSelection(SelectionType::Click);
	TestSelection(SelectionType::MiddleClick);
}
