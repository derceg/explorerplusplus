// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkRegistryStorage.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/BookmarkStorage.h"
#include "Bookmarks/BookmarkTree.h"
#include "../Helper/RegistrySettings.h"
#include <wil/resource.h>

namespace
{

namespace V2
{

const TCHAR bookmarksKeyPath[] = _T("Bookmarksv2");

void Load(HKEY parentKey, BookmarkTree *bookmarkTree);
void LoadPermanentFolder(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem,
	const std::wstring &name);
void LoadBookmarkChildren(HKEY parentKey, BookmarkTree *bookmarkTree,
	BookmarkItem *parentBookmarkItem);
std::unique_ptr<BookmarkItem> LoadBookmarkItem(HKEY key, BookmarkTree *bookmarkTree);

void Save(HKEY parentKey, BookmarkTree *bookmarkTree);
void SavePermanentFolder(HKEY parentKey, const BookmarkItem *bookmarkItem,
	const std::wstring &name);
void SaveBookmarkChildren(HKEY parentKey, const BookmarkItem *parentBookmarkItem);
void SaveBookmarkItem(HKEY key, const BookmarkItem *bookmarkItem);

}

// Note that there's no ability to save bookmarks in the v1 format, as they will
// always be saved in the v2 format.
namespace V1
{

const TCHAR bookmarksKeyPath[] = _T("Bookmarks");

void Load(HKEY parentKey, BookmarkTree *bookmarkTree);

void LoadBookmarkChildren(HKEY parentKey, BookmarkTree *bookmarkTree,
	BookmarkItem *parentBookmarkItem);
std::unique_ptr<BookmarkItem> LoadBookmarkItem(HKEY key, BookmarkTree *bookmarkTree,
	bool &showOnToolbarOutput);

}

}

namespace BookmarkRegistryStorage
{

void Load(HKEY applicationKey, BookmarkTree *bookmarkTree)
{
	// The V2 key always takes precedence (i.e. it will be used even if the V1
	// key exists).
	wil::unique_hkey bookmarksKey;
	LSTATUS res = RegOpenKeyEx(applicationKey, V2::bookmarksKeyPath, 0, KEY_READ, &bookmarksKey);

	if (res == ERROR_SUCCESS)
	{
		V2::Load(bookmarksKey.get(), bookmarkTree);
		return;
	}

	res = RegOpenKeyEx(applicationKey, V1::bookmarksKeyPath, 0, KEY_READ, &bookmarksKey);

	if (res == ERROR_SUCCESS)
	{
		V1::Load(bookmarksKey.get(), bookmarkTree);
		return;
	}
}

void Save(HKEY applicationKey, BookmarkTree *bookmarkTree)
{
	SHDeleteKey(applicationKey, V2::bookmarksKeyPath);

	wil::unique_hkey bookmarksKey;
	LSTATUS res = RegCreateKeyEx(applicationKey, V2::bookmarksKeyPath, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &bookmarksKey, nullptr);

	if (res == ERROR_SUCCESS)
	{
		V2::Save(bookmarksKey.get(), bookmarkTree);
	}
}

}

namespace
{

namespace V2
{

void Load(HKEY parentKey, BookmarkTree *bookmarkTree)
{
	LoadPermanentFolder(parentKey, bookmarkTree, bookmarkTree->GetBookmarksToolbarFolder(),
		BookmarkStorage::BOOKMARKS_TOOLBAR_NODE_NAME);
	LoadPermanentFolder(parentKey, bookmarkTree, bookmarkTree->GetBookmarksMenuFolder(),
		BookmarkStorage::BOOKMARKS_MENU_NODE_NAME);
	LoadPermanentFolder(parentKey, bookmarkTree, bookmarkTree->GetOtherBookmarksFolder(),
		BookmarkStorage::OTHER_BOOKMARKS_NODE_NAME);
}

void LoadPermanentFolder(HKEY parentKey, BookmarkTree *bookmarkTree, BookmarkItem *bookmarkItem,
	const std::wstring &name)
{
	wil::unique_hkey childKey;
	LSTATUS res = RegOpenKeyEx(parentKey, name.c_str(), 0, KEY_READ, &childKey);

	if (res == ERROR_SUCCESS)
	{
		FILETIME dateCreated;
		bool dateRes =
			RegistrySettings::ReadDateTime(childKey.get(), _T("DateCreated"), dateCreated);

		if (dateRes)
		{
			bookmarkItem->SetDateCreated(dateCreated);
		}

		FILETIME dateModified;
		dateRes = RegistrySettings::ReadDateTime(childKey.get(), _T("DateModified"), dateModified);

		if (dateRes)
		{
			bookmarkItem->SetDateModified(dateModified);
		}

		LoadBookmarkChildren(childKey.get(), bookmarkTree, bookmarkItem);
	}
}

void LoadBookmarkChildren(HKEY parentKey, BookmarkTree *bookmarkTree,
	BookmarkItem *parentBookmarkItem)
{
	wil::unique_hkey childKey;
	int index = 0;

	while (RegOpenKeyEx(parentKey, std::to_wstring(index).c_str(), 0, KEY_READ, &childKey)
		== ERROR_SUCCESS)
	{
		auto childBookmarkItem = LoadBookmarkItem(childKey.get(), bookmarkTree);
		bookmarkTree->AddBookmarkItem(parentBookmarkItem, std::move(childBookmarkItem), index);

		index++;
	}
}

std::unique_ptr<BookmarkItem> LoadBookmarkItem(HKEY key, BookmarkTree *bookmarkTree)
{
	DWORD type;
	RegistrySettings::ReadDword(key, _T("Type"), type);

	std::wstring guid;
	RegistrySettings::ReadString(key, _T("GUID"), guid);

	std::wstring name;
	RegistrySettings::ReadString(key, _T("Name"), name);

	std::optional<std::wstring> locationOptional;

	if (type == static_cast<int>(BookmarkItem::Type::Bookmark))
	{
		std::wstring location;
		RegistrySettings::ReadString(key, _T("Location"), location);

		locationOptional = location;
	}

	auto bookmarkItem = std::make_unique<BookmarkItem>(guid, name, locationOptional);

	FILETIME dateCreated;
	RegistrySettings::ReadDateTime(key, _T("DateCreated"), dateCreated);
	bookmarkItem->SetDateCreated(dateCreated);

	FILETIME dateModified;
	RegistrySettings::ReadDateTime(key, _T("DateModified"), dateModified);
	bookmarkItem->SetDateModified(dateModified);

	if (type == static_cast<int>(BookmarkItem::Type::Folder))
	{
		LoadBookmarkChildren(key, bookmarkTree, bookmarkItem.get());
	}

	return bookmarkItem;
}

void Save(HKEY parentKey, BookmarkTree *bookmarkTree)
{
	SavePermanentFolder(parentKey, bookmarkTree->GetBookmarksToolbarFolder(),
		BookmarkStorage::BOOKMARKS_TOOLBAR_NODE_NAME);
	SavePermanentFolder(parentKey, bookmarkTree->GetBookmarksMenuFolder(),
		BookmarkStorage::BOOKMARKS_MENU_NODE_NAME);
	SavePermanentFolder(parentKey, bookmarkTree->GetOtherBookmarksFolder(),
		BookmarkStorage::OTHER_BOOKMARKS_NODE_NAME);
}

void SavePermanentFolder(HKEY parentKey, const BookmarkItem *bookmarkItem, const std::wstring &name)
{
	wil::unique_hkey childKey;
	LSTATUS res = RegCreateKeyEx(parentKey, name.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
		KEY_WRITE, nullptr, &childKey, nullptr);

	if (res == ERROR_SUCCESS)
	{
		RegistrySettings::SaveDateTime(childKey.get(), _T("DateCreated"),
			bookmarkItem->GetDateCreated());
		RegistrySettings::SaveDateTime(childKey.get(), _T("DateModified"),
			bookmarkItem->GetDateModified());

		SaveBookmarkChildren(childKey.get(), bookmarkItem);
	}
}

void SaveBookmarkChildren(HKEY parentKey, const BookmarkItem *parentBookmarkItem)
{
	int index = 0;

	for (auto &child : parentBookmarkItem->GetChildren())
	{
		wil::unique_hkey childKey;
		LSTATUS res = RegCreateKeyEx(parentKey, std::to_wstring(index).c_str(), 0, nullptr,
			REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &childKey, nullptr);

		if (res == ERROR_SUCCESS)
		{
			SaveBookmarkItem(childKey.get(), child.get());

			index++;
		}
	}
}

void SaveBookmarkItem(HKEY key, const BookmarkItem *bookmarkItem)
{
	RegistrySettings::SaveDword(key, _T("Type"), static_cast<int>(bookmarkItem->GetType()));
	RegistrySettings::SaveString(key, _T("GUID"), bookmarkItem->GetGUID());
	RegistrySettings::SaveString(key, _T("Name"), bookmarkItem->GetName());

	if (bookmarkItem->GetType() == BookmarkItem::Type::Bookmark)
	{
		RegistrySettings::SaveString(key, _T("Location"), bookmarkItem->GetLocation());
	}

	RegistrySettings::SaveDateTime(key, _T("DateCreated"), bookmarkItem->GetDateCreated());
	RegistrySettings::SaveDateTime(key, _T("DateModified"), bookmarkItem->GetDateModified());

	if (bookmarkItem->GetType() == BookmarkItem::Type::Folder)
	{
		SaveBookmarkChildren(key, bookmarkItem);
	}
}

}

namespace V1
{

void Load(HKEY parentKey, BookmarkTree *bookmarkTree)
{
	LoadBookmarkChildren(parentKey, bookmarkTree, nullptr);
}

void LoadBookmarkChildren(HKEY parentKey, BookmarkTree *bookmarkTree,
	BookmarkItem *parentBookmarkItem)
{
	wil::unique_hkey childKey;
	int index = 0;

	while (RegOpenKeyEx(parentKey, std::to_wstring(index).c_str(), 0, KEY_READ, &childKey)
		== ERROR_SUCCESS)
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
				bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksToolbarFolder(),
					std::move(childBookmarkItem),
					bookmarkTree->GetBookmarksToolbarFolder()->GetChildren().size());
			}
			else
			{
				bookmarkTree->AddBookmarkItem(bookmarkTree->GetBookmarksMenuFolder(),
					std::move(childBookmarkItem),
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

std::unique_ptr<BookmarkItem> LoadBookmarkItem(HKEY key, BookmarkTree *bookmarkTree,
	bool &showOnToolbarOutput)
{
	DWORD type;
	RegistrySettings::ReadDword(key, _T("Type"), type);

	std::wstring name;
	RegistrySettings::ReadString(key, _T("Name"), name);

	DWORD showOnToolbar;
	RegistrySettings::ReadDword(key, _T("ShowOnBookmarksToolbar"), showOnToolbar);

	showOnToolbarOutput = showOnToolbar;

	std::optional<std::wstring> locationOptional;

	if (type == static_cast<int>(BookmarkStorage::BookmarkTypeV1::Bookmark))
	{
		std::wstring location;
		RegistrySettings::ReadString(key, _T("Location"), location);

		locationOptional = location;
	}

	auto bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt, name, locationOptional);

	if (type == static_cast<int>(BookmarkStorage::BookmarkTypeV1::Folder))
	{
		LoadBookmarkChildren(key, bookmarkTree, bookmarkItem.get());
	}

	return bookmarkItem;
}

}

}
