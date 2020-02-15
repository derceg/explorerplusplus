// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkRegistryStorage.h"
#include "BookmarkItem.h"
#include "BookmarkStorage.h"
#include "BookmarkTree.h"
#include "../Helper/RegistrySettings.h"
#include <wil/resource.h>

namespace V2
{
	const TCHAR bookmarksKeyPath[] = _T("Software\\Explorer++\\Bookmarksv2");

	void Load(HKEY parentKey, BookmarkTree *bookmarkTree);
	void LoadPermanentFolder(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem, const std::wstring &name);
	void LoadBookmarkChildren(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *parentBookmarkItem);
	std::unique_ptr<BookmarkItem> LoadBookmarkItem(HKEY key, BookmarkTree *bookmarkTree);

	void Save(HKEY parentKey, BookmarkTree *bookmarkTree);
	void SavePermanentFolder(HKEY parentKey, const BookmarkItem *bookmarkItem, const std::wstring &name);
	void SaveBookmarkChildren(HKEY parentKey, const BookmarkItem *parentBookmarkItem);
	void SaveBookmarkItem(HKEY key, const BookmarkItem *bookmarkItem);
}

// Note that there's no ability to save bookmarks in the v1 format, as they will
// always be saved in the v2 format.
namespace V1
{
	const TCHAR bookmarksKeyPath[] = _T("Software\\Explorer++\\Bookmarks");

	void Load(HKEY parentKey, BookmarkTree *bookmarkTree);

	void LoadBookmarkChildren(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *parentBookmarkItem);
	std::unique_ptr<BookmarkItem> LoadBookmarkItem(HKEY key, BookmarkTree *bookmarkTree, bool &showOnToolbarOutput);
}

void BookmarkRegistryStorage::Load(BookmarkTree *bookmarkTree)
{
	// The V2 key always takes precedence (i.e. it will be used even if the V1
	// key exists).
	wil::unique_hkey bookmarksKey;
	LONG res = RegOpenKeyEx(HKEY_CURRENT_USER, V2::bookmarksKeyPath, 0, KEY_READ, &bookmarksKey);

	if (res == ERROR_SUCCESS)
	{
		V2::Load(bookmarksKey.get(), bookmarkTree);
		return;
	}

	res = RegOpenKeyEx(HKEY_CURRENT_USER, V1::bookmarksKeyPath, 0, KEY_READ, &bookmarksKey);

	if (res == ERROR_SUCCESS)
	{
		V1::Load(bookmarksKey.get(), bookmarkTree);
		return;
	}
}

void V2::Load(HKEY parentKey, BookmarkTree *bookmarkTree)
{
	LoadPermanentFolder(parentKey, bookmarkTree, bookmarkTree->GetBookmarksToolbarFolder(),
		BookmarkStorage::BOOKMARKS_TOOLBAR_NODE_NAME);
	LoadPermanentFolder(parentKey, bookmarkTree, bookmarkTree->GetBookmarksMenuFolder(),
		BookmarkStorage::BOOKMARKS_MENU_NODE_NAME);
	LoadPermanentFolder(parentKey, bookmarkTree, bookmarkTree->GetOtherBookmarksFolder(),
		BookmarkStorage::OTHER_BOOKMARKS_NODE_NAME);
}

void V2::LoadPermanentFolder(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem, const std::wstring &name)
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

void V2::LoadBookmarkChildren(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *parentBookmarkItem)
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

std::unique_ptr<BookmarkItem> V2::LoadBookmarkItem(HKEY key, BookmarkTree *bookmarkTree)
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

void V1::Load(HKEY parentKey, BookmarkTree *bookmarkTree)
{
	LoadBookmarkChildren(parentKey, bookmarkTree, nullptr);
}

void V1::LoadBookmarkChildren(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *parentBookmarkItem)
{
	wil::unique_hkey childKey;
	int index = 0;

	while (RegOpenKeyEx(parentKey, std::to_wstring(index).c_str(), 0, KEY_READ, &childKey) == ERROR_SUCCESS)
	{
		bool showOnToolbar;
		auto childBookmarkItem = LoadBookmarkItem(childKey.get(), bookmarkTree, showOnToolbar);

		if (!parentBookmarkItem)
		{
			// In v1 of the bookmarks system, any bookmark items could be shown
			// on the bookmarks toolbar. For example, a bookmark could be shown
			// even if it's parent folder wasn't being shown.
			// In the current model, only bookmark items in the bookmarks
			// toolbar folder will be shown on the toolbar. Additionally, this
			// only applies to direct children of the folder and not any
			// children at a greater depth.
			// These two models are incompatible with each other. Therefore,
			// when loading v1 bookmark items, they'll only be added to the
			// toolbar folder if they're direct children of the root (and they
			// have the appropriate toolbar flag set).
			// There's no way to add a child item to the bookmarks toolbar,
			// without adding its top-level parent instead, so attempting to do
			// that would be more complicated and possibly surprising (since it
			// would mean that if you chose to display a grandchild bookmark on
			// the toolbar, but not its parents, the grandparent is what would
			// end up being displayed on the toolbar, not the grandchild).
			if (showOnToolbar)
			{
				bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksToolbarFolder(), std::move(childBookmarkItem),
					bookmarkTree->GetBookmarksToolbarFolder()->GetChildren().size());
			}
			else
			{
				bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(), std::move(childBookmarkItem),
					bookmarkTree->GetBookmarksMenuFolder()->GetChildren().size());
			}
		}
		else
		{
			bookmarkTree->AddBookmarkItem(parentBookmarkItem, std::move(childBookmarkItem),
				parentBookmarkItem->GetChildren().size());
		}

		index++;
	}
}

std::unique_ptr<BookmarkItem> V1::LoadBookmarkItem(HKEY key, BookmarkTree *bookmarkTree, bool &showOnToolbarOutput)
{
	DWORD type;
	NRegistrySettings::ReadDwordFromRegistry(key, _T("Type"), &type);

	std::wstring name;
	NRegistrySettings::ReadStringFromRegistry(key, _T("Name"), name);

	DWORD showOnToolbar;
	NRegistrySettings::ReadDwordFromRegistry(key, _T("ShowOnBookmarksToolbar"), &showOnToolbar);

	showOnToolbarOutput = showOnToolbar;

	std::optional<std::wstring> locationOptional;

	if (type == static_cast<int>(BookmarkStorage::BookmarkTypeV1::Bookmark))
	{
		std::wstring location;
		NRegistrySettings::ReadStringFromRegistry(key, _T("Location"), location);

		locationOptional = location;
	}

	auto bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt, name, locationOptional);

	if (type == static_cast<int>(BookmarkStorage::BookmarkTypeV1::Folder))
	{
		LoadBookmarkChildren(key, bookmarkTree, bookmarkItem.get());
	}

	return bookmarkItem;
}

void BookmarkRegistryStorage::Save(BookmarkTree *bookmarkTree)
{
	SHDeleteKey(HKEY_CURRENT_USER, V2::bookmarksKeyPath);

	wil::unique_hkey bookmarksKey;
	LONG res = RegCreateKeyEx(HKEY_CURRENT_USER, V2::bookmarksKeyPath, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &bookmarksKey, nullptr);

	if (res == ERROR_SUCCESS)
	{
		V2::Save(bookmarksKey.get(), bookmarkTree);
	}
}

void V2::Save(HKEY parentKey, BookmarkTree *bookmarkTree)
{
	SavePermanentFolder(parentKey, bookmarkTree->GetBookmarksToolbarFolder(),
		BookmarkStorage::BOOKMARKS_TOOLBAR_NODE_NAME);
	SavePermanentFolder(parentKey, bookmarkTree->GetBookmarksMenuFolder(),
		BookmarkStorage::BOOKMARKS_MENU_NODE_NAME);
	SavePermanentFolder(parentKey, bookmarkTree->GetOtherBookmarksFolder(),
		BookmarkStorage::OTHER_BOOKMARKS_NODE_NAME);
}

void V2::SavePermanentFolder(HKEY parentKey, const BookmarkItem *bookmarkItem, const std::wstring &name)
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

void V2::SaveBookmarkChildren(HKEY parentKey, const BookmarkItem *parentBookmarkItem)
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

void V2::SaveBookmarkItem(HKEY key, const BookmarkItem *bookmarkItem)
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