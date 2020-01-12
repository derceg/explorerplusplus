// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkDataExchange.h"
#include "BookmarkClipboard.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/iDataObject.h"
#include "../ThirdParty/cereal/archives/binary.hpp"
#include "../ThirdParty/cereal/types/memory.hpp"

FORMATETC BookmarkDataExchange::GetFormatEtc()
{
	static FORMATETC formatEtc = { static_cast<CLIPFORMAT>(BookmarkClipboard::GetClipboardFormat()), nullptr,
		DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	return formatEtc;
}

wil::com_ptr<IDataObject> BookmarkDataExchange::CreateDataObject(const std::unique_ptr<BookmarkItem> &bookmarkItem)
{
	FORMATETC formatEtc = GetFormatEtc();

	std::string data = SerializeBookmarkItem(bookmarkItem);
	auto global = WriteBinaryDataToGlobal(data);
	STGMEDIUM stgMedium = GetStgMediumForGlobal(global.get());

	wil::com_ptr<IDataObject> dataObject(CreateDataObject(&formatEtc, &stgMedium, 1));

	// TODO: Probably worth updating the code so that this doesn't need to be
	// done manually.
	// The IDataObject instance now owns the STGMEDIUM structure and is
	// responsible for freeing the memory associated with it.
	global.release();

	return dataObject;
}

std::string BookmarkDataExchange::SerializeBookmarkItem(const std::unique_ptr<BookmarkItem> &bookmarkItem)
{
	std::stringstream ss;
	cereal::BinaryOutputArchive outputArchive(ss);

	outputArchive(bookmarkItem);

	return ss.str();
}

std::unique_ptr<BookmarkItem> BookmarkDataExchange::DeserializeBookmarkItem(const std::string &data)
{
	std::stringstream ss(data);
	cereal::BinaryInputArchive inputArchive(ss);

	std::unique_ptr<BookmarkItem> bookmarkItem;
	inputArchive(bookmarkItem);

	return bookmarkItem;
}