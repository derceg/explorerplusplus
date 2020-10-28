// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/iDataObject.h"
#include "../ThirdParty/cereal/archives/binary.hpp"
#include "../ThirdParty/cereal/types/memory.hpp"

FORMATETC BookmarkDataExchange::GetFormatEtc()
{
	static FORMATETC formatEtc = { static_cast<CLIPFORMAT>(BookmarkClipboard::GetClipboardFormat()),
		nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	return formatEtc;
}

wil::com_ptr_nothrow<IDataObject> BookmarkDataExchange::CreateDataObject(
	const OwnedRefBookmarkItems &bookmarkItems)
{
	FORMATETC formatEtc = GetFormatEtc();

	std::string data = SerializeBookmarkItems(bookmarkItems);
	auto global = WriteBinaryDataToGlobal(data);
	STGMEDIUM stgMedium = GetStgMediumForGlobal(global.get());

	wil::com_ptr_nothrow<IDataObject> dataObject;
	dataObject.attach(CreateDataObject(&formatEtc, &stgMedium, 1));

	// TODO: Probably worth updating the code so that this doesn't need to be
	// done manually.
	// The IDataObject instance now owns the STGMEDIUM structure and is
	// responsible for freeing the memory associated with it.
	global.release();

	return dataObject;
}

std::string BookmarkDataExchange::SerializeBookmarkItems(const OwnedRefBookmarkItems &bookmarkItems)
{
	std::vector<std::string> serializedBookmarkItems;

	for (auto &bookmarkItem : bookmarkItems)
	{
		std::stringstream stringstream;
		cereal::BinaryOutputArchive outputArchive(stringstream);

		outputArchive(bookmarkItem.get());

		serializedBookmarkItems.push_back(stringstream.str());
	}

	std::stringstream mainStringstream;
	cereal::BinaryOutputArchive mainOutputArchive(mainStringstream);

	mainOutputArchive(serializedBookmarkItems);

	return mainStringstream.str();
}

BookmarkItems BookmarkDataExchange::DeserializeBookmarkItems(const std::string &data)
{
	std::stringstream mainStringstream(data);
	cereal::BinaryInputArchive mainInputArchive(mainStringstream);

	std::vector<std::string> serializedBookmarkItems;
	mainInputArchive(serializedBookmarkItems);

	BookmarkItems bookmarkItems;

	for (auto &serializedBookmarkItem : serializedBookmarkItems)
	{
		std::stringstream stringstream(serializedBookmarkItem);
		cereal::BinaryInputArchive inputArchive(stringstream);

		std::unique_ptr<BookmarkItem> bookmarkItem;
		inputArchive(bookmarkItem);

		bookmarkItems.push_back(std::move(bookmarkItem));
	}

	return bookmarkItems;
}