// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "../Helper/WinRTBaseWrapper.h"

namespace BookmarkDataExchange
{

FORMATETC GetFormatEtc();
winrt::com_ptr<IDataObject> CreateDataObject(const RawBookmarkItems &bookmarkItems);
std::string SerializeBookmarkItems(const RawBookmarkItems &bookmarkItems);
BookmarkItems DeserializeBookmarkItems(const std::string &data);

}
