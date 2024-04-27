// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellItemsMenu.h"
#include "BrowserWindowMock.h"
#include "IconFetcher.h"
#include "PopupMenuView.h"
#include "ShellTestHelper.h"
#include "../Helper/ShellHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace
{

std::wstring GetPathForItem(const std::wstring &name)
{
	return L"C:\\" + name;
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
		auto path = GetPathForItem(GetNameForItem(i));
		pidls.push_back(CreateSimplePidlForTest(path));
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

		AddResultCallback(callback);
	}

	void QueueIconTask(PCIDLIST_ABSOLUTE pidl, Callback callback) override
	{
		UNREFERENCED_PARAMETER(pidl);

		AddResultCallback(callback);
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

	void SetIgnoreRequests(bool ignoreRequests)
	{
		m_ignoreRequests = ignoreRequests;
	}

private:
	void AddResultCallback(Callback callback)
	{
		if (m_ignoreRequests)
		{
			return;
		}

		m_resultCallbacks.push_back(callback);
	}

	std::vector<Callback> m_resultCallbacks;
	bool m_ignoreRequests = false;
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

	std::unique_ptr<ShellItemsMenu> BuildMenu(MenuView *menuView,
		const std::vector<PidlAbsolute> &pidls, UINT menuStartId, UINT menuEndId)
	{
		return std::make_unique<ShellItemsMenu>(menuView, pidls, &m_browserWindow, &m_iconFetcher,
			menuStartId, menuEndId);
	}

	void CheckItemDetails(const MenuView &menuView, const std::vector<PidlAbsolute> &pidls)
	{
		ASSERT_EQ(static_cast<size_t>(menuView.GetItemCountForTesting()), pidls.size());

		for (size_t i = 0; i < pidls.size(); i++)
		{
			auto id = menuView.GetItemIdForTesting(static_cast<int>(i));
			auto name = GetNameForItem(i);
			EXPECT_EQ(menuView.GetItemTextForTesting(id), name);
			EXPECT_EQ(menuView.GetHelpTextForItem(id), GetPathForItem(name));
		}
	}

	BrowserWindowMock m_browserWindow;
	IconFetcherFake m_iconFetcher;
};

TEST_F(ShellItemsMenuTest, CheckItems)
{
	PopupMenuView popupMenu;
	auto pidls = BuildPidlCollection(3);
	auto menu = BuildMenu(&popupMenu, pidls);

	CheckItemDetails(popupMenu, pidls);
}

TEST_F(ShellItemsMenuTest, MaxItems)
{
	PopupMenuView popupMenu;
	auto pidls = BuildPidlCollection(3);
	auto menu = BuildMenu(&popupMenu, pidls, 1, 2);

	// The menu only has a single ID it can assign from the provided range of [1,2). So, although 3
	// items were passed in, only the first item should be added to the menu.
	EXPECT_EQ(popupMenu.GetItemCountForTesting(), 1);
	EXPECT_EQ(popupMenu.GetItemTextForTesting(popupMenu.GetItemIdForTesting(0)), GetNameForItem(0));
}

TEST_F(ShellItemsMenuTest, RebuildMenu)
{
	PopupMenuView popupMenu;
	auto pidls = BuildPidlCollection(3);
	auto menu = BuildMenu(&popupMenu, pidls);

	auto updatedPidls = BuildPidlCollection(5);
	menu->RebuildMenu(updatedPidls);

	CheckItemDetails(popupMenu, updatedPidls);
}

TEST_F(ShellItemsMenuTest, IconRetrievalAfterMenuRebuilt)
{
	PopupMenuView popupMenu;
	auto pidls = BuildPidlCollection(3);
	auto menu = BuildMenu(&popupMenu, pidls);

	// This will cause the icon fetcher to ignore icon tasks that are queued for the menu items
	// added as part of the rebuild.
	m_iconFetcher.SetIgnoreRequests(true);

	menu->RebuildMenu(pidls);

	std::vector<HBITMAP> originalBitmaps;

	for (int i = 0; i < popupMenu.GetItemCountForTesting(); i++)
	{
		auto bitmap = popupMenu.GetItemBitmapForTesting(popupMenu.GetItemIdForTesting(i));
		originalBitmaps.push_back(bitmap);
	}

	// This will trigger the icon callbacks for the original menu items (i.e. the menu items that
	// existed before the menu was rebuilt). This call should have no effect, since the original
	// menu items no longer exist.
	m_iconFetcher.TriggerPendingResultCallbacks();

	// As the callbacks that were triggered were for the original items on the menu, the images for
	// the new items shouldn't have changed.
	for (int i = 0; i < popupMenu.GetItemCountForTesting(); i++)
	{
		auto bitmap = popupMenu.GetItemBitmapForTesting(popupMenu.GetItemIdForTesting(i));
		EXPECT_EQ(bitmap, originalBitmaps.at(i));
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
			OpenItem(TypedEq<PCIDLIST_ABSOLUTE>(m_pidls[index]), disposition));

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
