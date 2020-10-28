// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include <wil/com.h>
#include <functional>

// This type is used when serializing multiple independent bookmark items and is
// needed for the following reasons:
//
// - cereal doesn't serialize raw pointers at all (it explicitly forbids them).
//   This means it's not possible to pass a list of BookmarkItem pointers to
//   serialize.
// - It's not possible to pass a list of unique_ptr<BookmarkItem>, because
//   unique_ptr's express unique ownership and the BookmarkItem's will likely be
//   owned by their parents.
// - Although it's possible to pass a reference to a single
//   unique_ptr<BookmarkItem>, it's not possible to pass a list of references,
//   since standard containers can't store references directly.
// - Although it would be possible to pass a list of raw pointers and then
//   retrieve the unique_ptr to each item via its parent, this would only work
//   in the case where each item has a parent. If you created a local
//   BookmarkItem, then passed a raw pointer to it, it wouldn't be possible to
//   retrieve a unique_ptr, since the item would have no parent. It wouldn't be
//   readily apparent by looking at the functions here that they would require
//   items with parents in order to retrieve the unique_ptr's.
// - The only other reasonable solution might be to copy each of the bookmark
//   item objects. This way, a list of unique_ptr<BookmarkItem> could be passed
//   in. However, this would mean that there would be some overhead in copying
//   the items and a method would have to be written to allow items to be
//   copied, which isn't normally something that needs to be done.
using OwnedRefBookmarkItems =
	std::vector<std::reference_wrapper<const std::unique_ptr<BookmarkItem>>>;

namespace BookmarkDataExchange
{
FORMATETC GetFormatEtc();
wil::com_ptr_nothrow<IDataObject> CreateDataObject(const OwnedRefBookmarkItems &bookmarkItems);
std::string SerializeBookmarkItems(const OwnedRefBookmarkItems &bookmarkItems);
BookmarkItems DeserializeBookmarkItems(const std::string &data);
}