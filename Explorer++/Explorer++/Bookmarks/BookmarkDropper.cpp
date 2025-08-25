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
#include <glog/logging.h>
#include <wil/com.h>

BookmarkDropper::BookmarkDropper(IDataObject *dataObject, DWORD allowedEffects,
	BookmarkTree *bookmarkTree) :
	m_dataObject(dataObject),
	m_allowedEffects(allowedEffects),
	m_bookmarkTree(bookmarkTree),
	m_blockDrop(false)
{
}

void BookmarkDropper::SetBlockDrop(bool blockDrop)
{
	m_blockDrop = blockDrop;
}

DWORD BookmarkDropper::GetDropEffect(const BookmarkItem *targetFolder, size_t index)
{
	DCHECK(targetFolder->IsFolder());

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

	if (extractedInfo.extractionSource == ExtractionSource::CustomFormat)
	{
		// When dropping an existing bookmark, it's only possible to move it.
		if (WI_IsFlagClear(m_allowedEffects, DROPEFFECT_MOVE))
		{
			return DROPEFFECT_NONE;
		}

		if (extractedInfo.bookmarkItems.size() == 1)
		{
			auto *existingBookmarkItem = m_bookmarkTree->MaybeGetBookmarkItemById(
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
	else
	{
		if (WI_IsFlagSet(m_allowedEffects, DROPEFFECT_COPY))
		{
			return DROPEFFECT_COPY;
		}
		else if (WI_IsFlagSet(m_allowedEffects, DROPEFFECT_LINK))
		{
			return DROPEFFECT_LINK;
		}
		else
		{
			// In this case, the only allowed effect must be DROPEFFECT_MOVE. The only items that
			// can be moved are bookmarks. It's not possible to move files/folders in to the
			// bookmarks tree, so the drop is blocked in that case.
			return DROPEFFECT_NONE;
		}
	}
}

DWORD BookmarkDropper::PerformDrop(BookmarkItem *targetFolder, size_t index)
{
	DCHECK(targetFolder->IsFolder());

	auto &extractedInfo = GetExtractedInfo();
	DWORD targetEffect = GetDropEffect(targetFolder, index);
	DWORD finalEffect = DROPEFFECT_NONE;
	size_t i = 0;

	for (auto &bookmarkItem : extractedInfo.bookmarkItems)
	{
		if (targetEffect == DROPEFFECT_COPY || targetEffect == DROPEFFECT_LINK)
		{
			m_bookmarkTree->AddBookmarkItem(targetFolder, std::move(bookmarkItem), index + i);
		}
		else if (targetEffect == DROPEFFECT_MOVE)
		{
			auto *existingBookmarkItem =
				m_bookmarkTree->MaybeGetBookmarkItemById(*bookmarkItem->GetOriginalGUID());

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

bool BookmarkDropper::CanDropBookmarkItemAtLocation(const BookmarkItem *bookmarkItem,
	const BookmarkItem *targetFolder, size_t index)
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
	ExtractionSource extractionSource;

	if (IsDropFormatAvailable(m_dataObject.get(), BookmarkDataExchange::GetFormatEtc()))
	{
		bookmarkItems = ExtractBookmarkItemsFromCustomFormat();
		extractionSource = ExtractionSource::CustomFormat;
	}
	else
	{
		bookmarkItems = MaybeExtractBookmarkItemsFromShellItems();
		extractionSource = ExtractionSource::Other;
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

BookmarkItems BookmarkDropper::MaybeExtractBookmarkItemsFromShellItems()
{
	wil::com_ptr_nothrow<IShellItemArray> dropShellItems;
	HRESULT hr =
		SHCreateShellItemArrayFromDataObject(m_dataObject.get(), IID_PPV_ARGS(&dropShellItems));

	if (FAILED(hr))
	{
		return {};
	}

	DWORD numItems;
	hr = dropShellItems->GetCount(&numItems);

	if (FAILED(hr))
	{
		return {};
	}

	BookmarkItems bookmarkItems;

	for (DWORD i = 0; i < numItems; i++)
	{
		wil::com_ptr_nothrow<IShellItem> shellItem;
		hr = dropShellItems->GetItemAt(i, &shellItem);

		if (FAILED(hr))
		{
			continue;
		}

		auto bookmarkItem = MaybeBuildBookmarkItemFromShellItem(shellItem.get());

		if (!bookmarkItem)
		{
			continue;
		}

		bookmarkItems.push_back(std::move(bookmarkItem));
	}

	return bookmarkItems;
}

std::unique_ptr<BookmarkItem> BookmarkDropper::MaybeBuildBookmarkItemFromShellItem(
	IShellItem *shellItem)
{
	wil::unique_cotaskmem_string displayName;
	HRESULT hr = shellItem->GetDisplayName(SIGDN_NORMALDISPLAY, &displayName);

	if (FAILED(hr))
	{
		return nullptr;
	}

	wil::unique_cotaskmem_string parsingPath;
	hr = shellItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &parsingPath);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return std::make_unique<BookmarkItem>(std::nullopt, displayName.get(), parsingPath.get());
}
