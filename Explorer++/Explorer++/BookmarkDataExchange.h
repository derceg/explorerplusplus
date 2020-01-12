// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkItem.h"
#include <wil/com.h>

namespace BookmarkDataExchange
{
	FORMATETC GetFormatEtc();
	wil::com_ptr<IDataObject> CreateDataObject(const std::unique_ptr<BookmarkItem> &bookmarkItem);
	std::string SerializeBookmarkItem(const std::unique_ptr<BookmarkItem> &bookmarkItem);
	std::unique_ptr<BookmarkItem> DeserializeBookmarkItem(const std::string &data);
}