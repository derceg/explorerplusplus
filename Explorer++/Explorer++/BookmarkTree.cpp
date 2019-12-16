// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkTree.h"
#include "BookmarkHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/RegistrySettings.h"
#include <wil/resource.h>

const WCHAR BOOKMARKS_TOOLBAR_NODE_NAME[] = L"BookmarksToolbar";
const WCHAR BOOKMARKS_MENU_NODE_NAME[] = L"BookmarksMenu";

BookmarkTree::BookmarkTree() :
	m_root(NBookmarkHelper::ROOT_GUID, ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_BOOKMARKS_ALLBOOKMARKS), std::nullopt)
{
	auto bookmarksToolbarFolder = std::make_unique<BookmarkItem>(NBookmarkHelper::TOOLBAR_GUID,
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_BOOKMARKS_BOOKMARKSTOOLBAR), std::nullopt);
	m_bookmarksToolbar = bookmarksToolbarFolder.get();
	m_root.AddChild(std::move(bookmarksToolbarFolder));

	auto bookmarksMenuFolder = std::make_unique<BookmarkItem>(NBookmarkHelper::MENU_GUID,
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_BOOKMARKS_BOOKMARKSMENU), std::nullopt);
	m_bookmarksMenu = bookmarksMenuFolder.get();
	m_root.AddChild(std::move(bookmarksMenuFolder));
}

BookmarkItem *BookmarkTree::GetRoot()
{
	return &m_root;
}

BookmarkItem *BookmarkTree::GetBookmarksToolbarFolder()
{
	return m_bookmarksToolbar;
}

BookmarkItem *BookmarkTree::GetBookmarksMenuFolder()
{
	return m_bookmarksMenu;
}

void BookmarkTree::LoadRegistrySettings(HKEY parentKey)
{
	LoadPermanentFolderFromRegistry(parentKey, m_bookmarksToolbar, BOOKMARKS_TOOLBAR_NODE_NAME);
	LoadPermanentFolderFromRegistry(parentKey, m_bookmarksMenu, BOOKMARKS_MENU_NODE_NAME);
}

void BookmarkTree::LoadPermanentFolderFromRegistry(HKEY parentKey, BookmarkItem *bookmarkItem, const std::wstring &name)
{
	wil::unique_hkey childKey;
	LONG res = RegOpenKeyEx(parentKey, name.c_str(), 0, KEY_READ, &childKey);

	if (res == ERROR_SUCCESS)
	{
		LoadBookmarkChildrenFromRegistry(childKey.get(), bookmarkItem);
	}
}

void BookmarkTree::LoadBookmarkChildrenFromRegistry(HKEY parentKey, BookmarkItem *parentBookmarkItem)
{
	wil::unique_hkey childKey;
	int index = 0;

	while (RegOpenKeyEx(parentKey, std::to_wstring(index).c_str(), 0, KEY_READ, &childKey) == ERROR_SUCCESS)
	{
		auto childBookmarkItem = LoadBookmarkItemFromRegistry(childKey.get());
		parentBookmarkItem->AddChild(std::move(childBookmarkItem));

		index++;
	}
}

std::unique_ptr<BookmarkItem> BookmarkTree::LoadBookmarkItemFromRegistry(HKEY key)
{
	DWORD type;
	NRegistrySettings::ReadDwordFromRegistry(key, _T("Type"), &type);

	std::wstring guid;
	NRegistrySettings::ReadStringFromRegistry(key, _T("GUID"), guid);

	std::wstring name;
	NRegistrySettings::ReadStringFromRegistry(key, _T("Name"), name);

	std::optional<std::wstring> locationOptional;

	if (type == static_cast<int>(BookmarkItem::Type::Bookmark))
	{
		std::wstring location;
		NRegistrySettings::ReadStringFromRegistry(key, _T("Location"), location);

		locationOptional = location;
	}

	auto bookmarkItem = std::make_unique<BookmarkItem>(guid, name, locationOptional);

	FILETIME dateCreated;
	NRegistrySettings::ReadDwordFromRegistry(key, _T("DateCreatedLow"), &dateCreated.dwLowDateTime);
	NRegistrySettings::ReadDwordFromRegistry(key, _T("DateCreatedHigh"), &dateCreated.dwHighDateTime);

	bookmarkItem->SetDateCreated(dateCreated);

	FILETIME dateModified;
	NRegistrySettings::ReadDwordFromRegistry(key, _T("DateModifiedLow"), &dateModified.dwLowDateTime);
	NRegistrySettings::ReadDwordFromRegistry(key, _T("DateModifiedHigh"), &dateModified.dwHighDateTime);

	bookmarkItem->SetDateModified(dateModified);

	if (type == static_cast<int>(BookmarkItem::Type::Folder))
	{
		LoadBookmarkChildrenFromRegistry(key, bookmarkItem.get());
	}

	return bookmarkItem;
}

void BookmarkTree::SaveRegistrySettings(HKEY parentKey)
{
	SavePermanentFolderToRegistry(parentKey, m_bookmarksToolbar, BOOKMARKS_TOOLBAR_NODE_NAME);
	SavePermanentFolderToRegistry(parentKey, m_bookmarksMenu, BOOKMARKS_MENU_NODE_NAME);
}

void BookmarkTree::SavePermanentFolderToRegistry(HKEY parentKey, const BookmarkItem *bookmarkItem, const std::wstring &name)
{
	wil::unique_hkey childKey;
	LONG res = RegCreateKeyEx(parentKey, name.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
		KEY_WRITE, nullptr, &childKey, nullptr);

	if (res == ERROR_SUCCESS)
	{
		SaveBookmarkChildrenToRegistry(childKey.get(), bookmarkItem);
	}
}

void BookmarkTree::SaveBookmarkChildrenToRegistry(HKEY parentKey, const BookmarkItem *parentBookmarkItem)
{
	int index = 0;

	for (auto &child : parentBookmarkItem->GetChildren())
	{
		wil::unique_hkey childKey;
		LONG res = RegCreateKeyEx(parentKey, std::to_wstring(index).c_str(), 0,
			nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &childKey, nullptr);

		if (res == ERROR_SUCCESS)
		{
			SaveBookmarkItemToRegistry(childKey.get(), child.get());

			index++;
		}
	}
}

void BookmarkTree::SaveBookmarkItemToRegistry(HKEY key, const BookmarkItem *bookmarkItem)
{
	NRegistrySettings::SaveDwordToRegistry(key, _T("Type"), static_cast<int>(bookmarkItem->GetType()));
	NRegistrySettings::SaveStringToRegistry(key, _T("GUID"), bookmarkItem->GetGUID().c_str());
	NRegistrySettings::SaveStringToRegistry(key, _T("Name"), bookmarkItem->GetName().c_str());

	if (bookmarkItem->GetType() == BookmarkItem::Type::Bookmark)
	{
		NRegistrySettings::SaveStringToRegistry(key, _T("Location"), bookmarkItem->GetLocation().c_str());
	}

	NRegistrySettings::SaveDwordToRegistry(key, _T("DateCreatedLow"), bookmarkItem->GetDateCreated().dwLowDateTime);
	NRegistrySettings::SaveDwordToRegistry(key, _T("DateCreatedHigh"), bookmarkItem->GetDateCreated().dwHighDateTime);
	NRegistrySettings::SaveDwordToRegistry(key, _T("DateModifiedLow"), bookmarkItem->GetDateModified().dwLowDateTime);
	NRegistrySettings::SaveDwordToRegistry(key, _T("DateModifiedHigh"), bookmarkItem->GetDateModified().dwHighDateTime);

	if (bookmarkItem->GetType() == BookmarkItem::Type::Folder)
	{
		SaveBookmarkChildrenToRegistry(key, bookmarkItem);
	}
}