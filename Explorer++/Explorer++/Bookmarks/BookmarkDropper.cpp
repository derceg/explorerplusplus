// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkDropper.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/ShellHelper.h"

BookmarkDropper::BookmarkDropper(IDataObject *dataObject, BookmarkTree *bookmarkTree) :
	m_dataObject(dataObject),
	m_bookmarkTree(bookmarkTree),
	m_blockDrop(false)
{
}

void BookmarkDropper::SetBlockDrop(bool blockDrop)
{
	m_blockDrop = blockDrop;
}

DWORD BookmarkDropper::GetDropEffect(BookmarkItem *targetFolder, size_t index)
{
	assert(targetFolder->IsFolder());

	if (m_blockDrop)
	{
		return DROPEFFECT_NONE;
	}

	if (!m_bookmarkTree->CanAddChildren(targetFolder))
	{
		return DROPEFFECT_NONE;
	}

	auto &extractedInfo = GetExtractedInfo();

	if (extractedInfo.bookmarkItems.empty())
	{
		return DROPEFFECT_NONE;
	}

	if (*extractedInfo.extractionSource == ExtractionSource::HDrop)
	{
		return DROPEFFECT_COPY;
	}
	else if (*extractedInfo.extractionSource == ExtractionSource::CustomFormat)
	{
		if (extractedInfo.bookmarkItems.size() == 1)
		{
			auto existingBookmarkItem = BookmarkHelper::GetBookmarkItemById(m_bookmarkTree,
				*extractedInfo.bookmarkItems[0]->GetOriginalGUID());

			if (!existingBookmarkItem
				|| (existingBookmarkItem
					&& !CanDropBookmarkItemAtLocation(existingBookmarkItem, targetFolder, index)))
			{
				return DROPEFFECT_NONE;
			}
		}

		return DROPEFFECT_MOVE;
	}

	return DROPEFFECT_NONE;
}

DWORD BookmarkDropper::PerformDrop(BookmarkItem *targetFolder, size_t index)
{
	assert(targetFolder->IsFolder());

	auto &extractedInfo = GetExtractedInfo();
	DWORD targetEffect = GetDropEffect(targetFolder, index);
	DWORD finalEffect = DROPEFFECT_NONE;
	size_t i = 0;

	for (auto &bookmarkItem : extractedInfo.bookmarkItems)
	{
		if (targetEffect == DROPEFFECT_COPY)
		{
			m_bookmarkTree->AddBookmarkItem(targetFolder, std::move(bookmarkItem), index + i);
		}
		else if (targetEffect == DROPEFFECT_MOVE)
		{
			auto existingBookmarkItem = BookmarkHelper::GetBookmarkItemById(m_bookmarkTree,
				*bookmarkItem->GetOriginalGUID());

			if (existingBookmarkItem
				&& CanDropBookmarkItemAtLocation(existingBookmarkItem, targetFolder, index + i))
			{
				m_bookmarkTree->MoveBookmarkItem(existingBookmarkItem, targetFolder, index + i);
			}
		}

		i++;
	}

	if (!extractedInfo.bookmarkItems.empty())
	{
		finalEffect = targetEffect;
	}

	return finalEffect;
}

bool BookmarkDropper::CanDropBookmarkItemAtLocation(BookmarkItem *bookmarkItem,
	BookmarkItem *targetFolder, size_t index)
{
	if (bookmarkItem->IsFolder() && BookmarkHelper::IsAncestor(targetFolder, bookmarkItem))
	{
		return false;
	}

	if (bookmarkItem->GetParent() == targetFolder)
	{
		auto currentIndex = bookmarkItem->GetParent()->GetChildIndex(bookmarkItem);

		if ((index == currentIndex) || (index == currentIndex + 1))
		{
			return false;
		}
	}

	return true;
}

BookmarkDropper::ExtractedInfo &BookmarkDropper::GetExtractedInfo()
{
	if (!m_extractedInfo)
	{
		m_extractedInfo = ExtractBookmarkItems();
	}

	return *m_extractedInfo;
}

BookmarkDropper::ExtractedInfo BookmarkDropper::ExtractBookmarkItems()
{
	BookmarkItems bookmarkItems;
	std::optional<ExtractionSource> extractionSource;

	if (IsDropFormatAvailable(m_dataObject, BookmarkDataExchange::GetFormatEtc()))
	{
		bookmarkItems = ExtractBookmarkItemsFromCustomFormat();
		extractionSource = ExtractionSource::CustomFormat;
	}
	else if (IsDropFormatAvailable(m_dataObject, GetDroppedFilesFormatEtc()))
	{
		bookmarkItems = ExtractBookmarkItemsFromHDrop();
		extractionSource = ExtractionSource::HDrop;
	}

	return { std::move(bookmarkItems), extractionSource };
}

BookmarkItems BookmarkDropper::ExtractBookmarkItemsFromCustomFormat()
{
	FORMATETC formatEtc = BookmarkDataExchange::GetFormatEtc();
	wil::unique_stg_medium stgMedium;
	HRESULT hr = m_dataObject->GetData(&formatEtc, &stgMedium);

	if (hr != S_OK)
	{
		return {};
	}

	auto data = ReadBinaryDataFromGlobal(stgMedium.hGlobal);

	if (!data)
	{
		return {};
	}

	return BookmarkDataExchange::DeserializeBookmarkItems(*data);
}

BookmarkItems BookmarkDropper::ExtractBookmarkItemsFromHDrop()
{
	BookmarkItems bookmarkItems;
	auto droppedFiles = ExtractDroppedFilesList(m_dataObject);

	for (auto &droppedFile : droppedFiles)
	{
		if (!PathIsDirectory(droppedFile.c_str()))
		{
			continue;
		}

		std::wstring displayName;
		GetDisplayName(droppedFile.c_str(), SHGDN_INFOLDER, displayName);

		auto bookmarkItem =
			std::make_unique<BookmarkItem>(std::nullopt, displayName, droppedFile.c_str());
		bookmarkItems.push_back(std::move(bookmarkItem));
	}

	return bookmarkItems;
}
