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
const WCHAR OTHER_BOOKMARKS_NODE_NAME[] = L"OtherBookmarks";

BookmarkTree::BookmarkTree() :
	m_root(ROOT_FOLDER_GUID, ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_BOOKMARKS_ALLBOOKMARKS), std::nullopt)
{
	auto bookmarksToolbarFolder = std::make_unique<BookmarkItem>(TOOLBAR_FOLDER_GUID,
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_BOOKMARKS_BOOKMARKSTOOLBAR), std::nullopt);
	m_bookmarksToolbar = bookmarksToolbarFolder.get();
	m_root.AddChild(std::move(bookmarksToolbarFolder));

	auto bookmarksMenuFolder = std::make_unique<BookmarkItem>(MENU_FOLDER_GUID,
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_BOOKMARKS_BOOKMARKSMENU), std::nullopt);
	m_bookmarksMenu = bookmarksMenuFolder.get();
	m_root.AddChild(std::move(bookmarksMenuFolder));

	auto otherBookmarksFolder = std::make_unique<BookmarkItem>(OTHER_FOLDER_GUID,
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_BOOKMARKS_OTHER_BOOKMARKS), std::nullopt);
	m_otherBookmarks = otherBookmarksFolder.get();
	m_root.AddChild(std::move(otherBookmarksFolder));
}

BookmarkItem *BookmarkTree::GetRoot()
{
	return &m_root;
}

BookmarkItem *BookmarkTree::GetBookmarksToolbarFolder()
{
	return m_bookmarksToolbar;
}

const BookmarkItem *BookmarkTree::GetBookmarksToolbarFolder() const
{
	return m_bookmarksToolbar;
}

BookmarkItem *BookmarkTree::GetBookmarksMenuFolder()
{
	return m_bookmarksMenu;
}

const BookmarkItem *BookmarkTree::GetBookmarksMenuFolder() const
{
	return m_bookmarksMenu;
}

BookmarkItem *BookmarkTree::GetOtherBookmarksFolder()
{
	return m_otherBookmarks;
}

const BookmarkItem *BookmarkTree::GetOtherBookmarksFolder() const
{
	return m_otherBookmarks;
}

void BookmarkTree::AddBookmarkItem(BookmarkItem *parent, std::unique_ptr<BookmarkItem> bookmarkItem, size_t index)
{
	if (!CanAddChildren(parent))
	{
		assert(false);
		return;
	}

	bookmarkItem->VisitRecursively([this] (BookmarkItem *currentItem) {
		currentItem->ClearOriginalGUID();

		// Adds an observer to each bookmark item that's being added. This is
		// needed so that this class can broadcast an event whenever an
		// individual bookmark item is updated.
		currentItem->updatedSignal.AddObserver(std::bind(&BookmarkTree::OnBookmarkItemUpdated, this,
			std::placeholders::_1, std::placeholders::_2), boost::signals2::at_front);
	});

	if (index > parent->GetChildren().size())
	{
		index = parent->GetChildren().size();
	}

	BookmarkItem *rawBookmarkItem = bookmarkItem.get();
	parent->AddChild(std::move(bookmarkItem), index);
	bookmarkItemAddedSignal.m_signal(*rawBookmarkItem, index);
}

void BookmarkTree::MoveBookmarkItem(BookmarkItem *bookmarkItem, BookmarkItem *newParent, size_t index)
{
	if (!CanAddChildren(newParent) || IsPermanentNode(bookmarkItem))
	{
		assert(false);
		return;
	}

	BookmarkItem *oldParent = bookmarkItem->GetParent();
	size_t oldIndex = oldParent->GetChildIndex(bookmarkItem);

	if (index > newParent->GetChildren().size())
	{
		index = newParent->GetChildren().size();
	}

	if (oldParent == newParent && index > oldIndex)
	{
		index--;
	}

	if (oldParent == newParent && index == oldIndex)
	{
		return;
	}

	auto item = oldParent->RemoveChild(oldIndex);
	newParent->AddChild(std::move(item), index);

	bookmarkItemMovedSignal.m_signal(bookmarkItem, oldParent, oldIndex,
		newParent, index);
}

void BookmarkTree::RemoveBookmarkItem(BookmarkItem *bookmarkItem)
{
	if (IsPermanentNode(bookmarkItem))
	{
		assert(false);
		return;
	}

	bookmarkItemPreRemovalSignal.m_signal(*bookmarkItem);

	BookmarkItem *parent = bookmarkItem->GetParent();
	assert(bookmarkItem->GetParent() != nullptr);

	std::wstring guid = bookmarkItem->GetGUID();

	size_t childIndex = parent->GetChildIndex(bookmarkItem);
	parent->RemoveChild(childIndex);
	bookmarkItemRemovedSignal.m_signal(guid);
}

void BookmarkTree::OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType)
{
	bookmarkItemUpdatedSignal.m_signal(bookmarkItem, propertyType);
}

bool BookmarkTree::CanAddChildren(const BookmarkItem *bookmarkItem) const
{
	return bookmarkItem != &m_root;
}

bool BookmarkTree::IsPermanentNode(const BookmarkItem *bookmarkItem) const
{
	if (bookmarkItem == &m_root
		|| bookmarkItem == m_bookmarksToolbar
		|| bookmarkItem == m_bookmarksMenu
		|| bookmarkItem == m_otherBookmarks)
	{
		return true;
	}

	return false;
}

void BookmarkTree::LoadRegistrySettings(HKEY parentKey)
{
	LoadPermanentFolderFromRegistry(parentKey, m_bookmarksToolbar, BOOKMARKS_TOOLBAR_NODE_NAME);
	LoadPermanentFolderFromRegistry(parentKey, m_bookmarksMenu, BOOKMARKS_MENU_NODE_NAME);
	LoadPermanentFolderFromRegistry(parentKey, m_otherBookmarks, OTHER_BOOKMARKS_NODE_NAME);
}

void BookmarkTree::LoadPermanentFolderFromRegistry(HKEY parentKey, BookmarkItem *bookmarkItem, const std::wstring &name)
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
		AddBookmarkItem(parentBookmarkItem, std::move(childBookmarkItem), index);

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
	NRegistrySettings::ReadDateTime(key, _T("DateCreated"), dateCreated);
	bookmarkItem->SetDateCreated(dateCreated);

	FILETIME dateModified;
	NRegistrySettings::ReadDateTime(key, _T("DateModified"), dateModified);
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
	SavePermanentFolderToRegistry(parentKey, m_otherBookmarks, OTHER_BOOKMARKS_NODE_NAME);
}

void BookmarkTree::SavePermanentFolderToRegistry(HKEY parentKey, const BookmarkItem *bookmarkItem, const std::wstring &name)
{
	wil::unique_hkey childKey;
	LONG res = RegCreateKeyEx(parentKey, name.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
		KEY_WRITE, nullptr, &childKey, nullptr);

	if (res == ERROR_SUCCESS)
	{
		NRegistrySettings::SaveDateTime(childKey.get(), _T("DateCreated"), bookmarkItem->GetDateCreated());
		NRegistrySettings::SaveDateTime(childKey.get(), _T("DateModified"), bookmarkItem->GetDateModified());

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

	NRegistrySettings::SaveDateTime(key, _T("DateCreated"), bookmarkItem->GetDateCreated());
	NRegistrySettings::SaveDateTime(key, _T("DateModified"), bookmarkItem->GetDateModified());

	if (bookmarkItem->GetType() == BookmarkItem::Type::Folder)
	{
		SaveBookmarkChildrenToRegistry(key, bookmarkItem);
	}
}