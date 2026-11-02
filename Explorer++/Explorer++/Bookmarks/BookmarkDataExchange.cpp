// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/DragDropHelper.h"
#include <nlohmann/json.hpp>

namespace
{

constexpr int BOOKMARK_DATA_EXCHANGE_VERSION = 1;

}

FORMATETC BookmarkDataExchange::GetFormatEtc()
{
	static FORMATETC formatEtc = { static_cast<CLIPFORMAT>(BookmarkClipboard::GetClipboardFormat()),
		nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	return formatEtc;
}

winrt::com_ptr<IDataObject> BookmarkDataExchange::CreateDataObject(
	const RawBookmarkItems &bookmarkItems)
{
	FORMATETC formatEtc = GetFormatEtc();

	std::string data = SerializeBookmarkItems(bookmarkItems);
	auto global = WriteBinaryDataToGlobal(data);
	auto stgMedium = GetStgMediumForGlobal(std::move(global));

	auto dataObject = winrt::make<DataObjectImpl>();
	HRESULT hr = MoveStorageToObject(dataObject.get(), &formatEtc, std::move(stgMedium));
	DCHECK(SUCCEEDED(hr));

	return dataObject;
}

std::string BookmarkDataExchange::SerializeBookmarkItems(const RawBookmarkItems &bookmarkItems)
{
	nlohmann::json root = { { "version", BOOKMARK_DATA_EXCHANGE_VERSION },
		{ "items", nlohmann::json::array() } };

	for (const auto *bookmarkItem : bookmarkItems)
	{
		root["items"].push_back(bookmarkItem->ToSerializedData());
	}

	return root.dump();
}

BookmarkItems BookmarkDataExchange::DeserializeBookmarkItems(const std::string &data)
{
	std::vector<BookmarkItem::SerializedData> serializedItems;

	try
	{
		const auto root = nlohmann::json::parse(data);

		auto version = root.at("version").get<int>();

		if (version != BOOKMARK_DATA_EXCHANGE_VERSION)
		{
			return {};
		}

		serializedItems = root.at("items").get<std::vector<BookmarkItem::SerializedData>>();
	}
	catch (const nlohmann::json::exception &)
	{
		return {};
	}

	BookmarkItems bookmarkItems;

	for (const auto &serializedItem : serializedItems)
	{
		auto bookmarkItem = BookmarkItem::MaybeCreateFromSerializedData(serializedItem);

		if (!bookmarkItem)
		{
			continue;
		}

		bookmarkItems.push_back(std::move(bookmarkItem));
	}

	return bookmarkItems;
}
