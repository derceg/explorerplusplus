// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkRegistryStorage.h"
#include "BookmarkItem.h"
#include "BookmarkStorage.h"
#include "../Helper/RegistrySettings.h"
#include <wil/resource.h>

void LoadPermanentFolder(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem, const std::wstring &name);
void LoadBookmarkChildren(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *parentBookmarkItem);
std::unique_ptr<BookmarkItem> LoadBookmarkItem(HKEY key, BookmarkTree *bookmarkTree);

void SavePermanentFolder(HKEY parentKey, const BookmarkItem *bookmarkItem, const std::wstring &name);
void SaveBookmarkChildren(HKEY parentKey, const BookmarkItem *parentBookmarkItem);
void SaveBookmarkItem(HKEY key, const BookmarkItem *bookmarkItem);

void BookmarkRegistryStorage::Load(HKEY parentKey, BookmarkTree *bookmarkTree)
{
	LoadPermanentFolder(parentKey, bookmarkTree, bookmarkTree->GetBookmarksToolbarFolder(), BOOKMARKS_TOOLBAR_NODE_NAME);
	LoadPermanentFolder(parentKey, bookmarkTree, bookmarkTree->GetBookmarksMenuFolder(), BOOKMARKS_MENU_NODE_NAME);
	LoadPermanentFolder(parentKey, bookmarkTree, bookmarkTree->GetOtherBookmarksFolder(), OTHER_BOOKMARKS_NODE_NAME);
}

void LoadPermanentFolder(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem, const std::wstring &name)
{
	wil::unique_hkey childKey;
	LONG res = RegOpenKeyEx(parentKey, name.c_str(), 0, KEY_READ, &childKey);

	if (res == ERROR_SUCCESS)
	{
		FILETIME dateCreated;
		bool dateRes = NRegistrySettings::ReadDateTime(childKey.get(), _T("DateCreated"), dateCreated);

		if (dateRes)
		{
			bookmarkItem->SetDateCreated(dateCreated);
		}

		FILETIME dateModified;
		dateRes = NRegistrySettings::ReadDateTime(childKey.get(), _T("DateModified"), dateModified);

		if (dateRes)
		{
			bookmarkItem->SetDateModified(dateModified);
		}

		LoadBookmarkChildren(childKey.get(), bookmarkTree, bookmarkItem);
	}
}

void LoadBookmarkChildren(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *parentBookmarkItem)
{
	wil::unique_hkey childKey;
	int index = 0;

	while (RegOpenKeyEx(parentKey, std::to_wstring(index).c_str(), 0, KEY_READ, &childKey) == ERROR_SUCCESS)
	{
		auto childBookmarkItem = LoadBookmarkItem(childKey.get(), bookmarkTree);
		bookmarkTree->AddBookmarkItem(parentBookmarkItem, std::move(childBookmarkItem), index);

		index++;
	}
}

std::unique_ptr<BookmarkItem> LoadBookmarkItem(HKEY key, BookmarkTree *bookmarkTree)
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
	NRegistrySettings::ReadDateTime(key, _T("DateCreated"), dateCreated);
	bookmarkItem->SetDateCreated(dateCreated);

	FILETIME dateModified;
	NRegistrySettings::ReadDateTime(key, _T("DateModified"), dateModified);
	bookmarkItem->SetDateModified(dateModified);

	if (type == static_cast<int>(BookmarkItem::Type::Folder))
	{
		LoadBookmarkChildren(key, bookmarkTree, bookmarkItem.get());
	}

	return bookmarkItem;
}

void BookmarkRegistryStorage::Save(HKEY parentKey, BookmarkTree *bookmarkTree)
{
	SavePermanentFolder(parentKey, bookmarkTree->GetBookmarksToolbarFolder(), BOOKMARKS_TOOLBAR_NODE_NAME);
	SavePermanentFolder(parentKey, bookmarkTree->GetBookmarksMenuFolder(), BOOKMARKS_MENU_NODE_NAME);
	SavePermanentFolder(parentKey, bookmarkTree->GetOtherBookmarksFolder(), OTHER_BOOKMARKS_NODE_NAME);
}

void SavePermanentFolder(HKEY parentKey, const BookmarkItem *bookmarkItem, const std::wstring &name)
{
	wil::unique_hkey childKey;
	LONG res = RegCreateKeyEx(parentKey, name.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
		KEY_WRITE, nullptr, &childKey, nullptr);

	if (res == ERROR_SUCCESS)
	{
		NRegistrySettings::SaveDateTime(childKey.get(), _T("DateCreated"), bookmarkItem->GetDateCreated());
		NRegistrySettings::SaveDateTime(childKey.get(), _T("DateModified"), bookmarkItem->GetDateModified());

		SaveBookmarkChildren(childKey.get(), bookmarkItem);
	}
}

void SaveBookmarkChildren(HKEY parentKey, const BookmarkItem *parentBookmarkItem)
{
	int index = 0;

	for (auto &child : parentBookmarkItem->GetChildren())
	{
		wil::unique_hkey childKey;
		LONG res = RegCreateKeyEx(parentKey, std::to_wstring(index).c_str(), 0,
			nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &childKey, nullptr);

		if (res == ERROR_SUCCESS)
		{
			SaveBookmarkItem(childKey.get(), child.get());

			index++;
		}
	}
}

void SaveBookmarkItem(HKEY key, const BookmarkItem *bookmarkItem)
{
	NRegistrySettings::SaveDwordToRegistry(key, _T("Type"), static_cast<int>(bookmarkItem->GetType()));
	NRegistrySettings::SaveStringToRegistry(key, _T("GUID"), bookmarkItem->GetGUID().c_str());
	NRegistrySettings::SaveStringToRegistry(key, _T("Name"), bookmarkItem->GetName().c_str());

	if (bookmarkItem->GetType() == BookmarkItem::Type::Bookmark)
	{
		NRegistrySettings::SaveStringToRegistry(key, _T("Location"), bookmarkItem->GetLocation().c_str());
	}

	NRegistrySettings::SaveDateTime(key, _T("DateCreated"), bookmarkItem->GetDateCreated());
	NRegistrySettings::SaveDateTime(key, _T("DateModified"), bookmarkItem->GetDateModified());

	if (bookmarkItem->GetType() == BookmarkItem::Type::Folder)
	{
		SaveBookmarkChildren(key, bookmarkItem);
	}
}