// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkDropInfo.h"
#include "BookmarkDataExchange.h"
#include "BookmarkHelper.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/ShellHelper.h"

BookmarkDropInfo::BookmarkDropInfo(IDataObject *dataObject, BookmarkTree *bookmarkTree) :
	m_dataObject(dataObject),
	m_bookmarkTree(bookmarkTree)
{

}

DWORD BookmarkDropInfo::GetDropEffect(BookmarkItem *parentFolder)
{
	assert(parentFolder->IsFolder());

	if (!m_bookmarkTree->CanAddChildren(parentFolder))
	{
		return DROPEFFECT_NONE;
	}

	if (!m_dropEffect)
	{
		m_dropEffect = DetermineDropEffect();
	}

	return *m_dropEffect;
}

DWORD BookmarkDropInfo::DetermineDropEffect()
{
	if (IsDropFormatAvailable(m_dataObject, BookmarkDataExchange::GetFormatEtc()))
	{
		// TODO: Should block drop if a folder is dragged over itself.
		return DROPEFFECT_MOVE;
	}
	else if (IsDropFormatAvailable(m_dataObject, GetDroppedFilesFormatEtc()))
	{
		auto droppedFiles = ExtractDroppedFilesList(m_dataObject);

		bool allFolders = std::all_of(droppedFiles.begin(), droppedFiles.end(), [] (const std::wstring &path) {
			return PathIsDirectory(path.c_str());
		});

		if (!droppedFiles.empty() && allFolders)
		{
			return DROPEFFECT_COPY;
		}
	}

	return DROPEFFECT_NONE;
}

DWORD BookmarkDropInfo::PerformDrop(BookmarkItem *parentFolder, size_t position)
{
	assert(parentFolder->IsFolder());

	BookmarkItems bookmarkItems = ExtractBookmarkItems();
	DWORD targetEffect = GetDropEffect(parentFolder);
	DWORD finalEffect = DROPEFFECT_NONE;
	size_t i = 0;

	for (auto &bookmarkItem : bookmarkItems)
	{
		if (targetEffect == DROPEFFECT_COPY)
		{
			m_bookmarkTree->AddBookmarkItem(parentFolder, std::move(bookmarkItem), position + i);
		}
		else if (targetEffect == DROPEFFECT_MOVE)
		{
			auto exingBookmarkItem = BookmarkHelper::GetBookmarkItemById(m_bookmarkTree, *bookmarkItem->GetOriginalGUID());

			if (exingBookmarkItem)
			{
				m_bookmarkTree->MoveBookmarkItem(exingBookmarkItem, parentFolder, position + i);
			}
		}

		i++;
	}

	if (!bookmarkItems.empty())
	{
		finalEffect = targetEffect;
	}

	return finalEffect;
}

BookmarkItems BookmarkDropInfo::ExtractBookmarkItems()
{
	if (IsDropFormatAvailable(m_dataObject, BookmarkDataExchange::GetFormatEtc()))
	{
		auto bookmarkItem = ExtractBookmarkItemFromCustomFormat();

		if (bookmarkItem)
		{
			BookmarkItems bookmarkItems;
			bookmarkItems.push_back(std::move(bookmarkItem));
			return bookmarkItems;
		}
	}
	else if (IsDropFormatAvailable(m_dataObject, GetDroppedFilesFormatEtc()))
	{
		return ExtractBookmarkItemsFromHDrop();
	}

	return {};
}

std::unique_ptr<BookmarkItem> BookmarkDropInfo::ExtractBookmarkItemFromCustomFormat()
{
	FORMATETC formatEtc = BookmarkDataExchange::GetFormatEtc();
	wil::unique_stg_medium stgMedium;
	HRESULT hr = m_dataObject->GetData(&formatEtc, &stgMedium);

	if (hr != S_OK)
	{
		return nullptr;
	}

	auto data = ReadBinaryDataFromGlobal(stgMedium.hGlobal);

	if (!data)
	{
		return nullptr;
	}

	return BookmarkDataExchange::DeserializeBookmarkItem(*data);
}

BookmarkItems BookmarkDropInfo::ExtractBookmarkItemsFromHDrop()
{
	BookmarkItems bookmarkItems;
	auto droppedFiles = ExtractDroppedFilesList(m_dataObject);

	for (auto &droppedFile : droppedFiles)
	{
		if (!PathIsDirectory(droppedFile.c_str()))
		{
			continue;
		}

		TCHAR displayName[MAX_PATH];
		GetDisplayName(droppedFile.c_str(), displayName, SIZEOF_ARRAY(displayName), SHGDN_INFOLDER);

		auto bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt, displayName, droppedFile.c_str());
		bookmarkItems.push_back(std::move(bookmarkItem));
	}

	return bookmarkItems;
}