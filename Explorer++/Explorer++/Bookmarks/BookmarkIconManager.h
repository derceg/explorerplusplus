// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <wil/resource.h>
#include <memory>

class BookmarkItem;
class IconFetcher;
__interface IExplorerplusplus;

class BookmarkIconManager
{
public:
	using IconAvailableCallback = std::function<void(std::wstring_view guid, int iconIndex)>;

	BookmarkIconManager(IExplorerplusplus *expp, IconFetcher *iconFetcher,
		IconAvailableCallback callback, int iconWidth, int iconHeight);
	~BookmarkIconManager();

	BookmarkIconManager(const BookmarkIconManager &other) = delete;
	BookmarkIconManager(const BookmarkIconManager &&other) = delete;
	BookmarkIconManager &operator=(const BookmarkIconManager &other) = delete;
	BookmarkIconManager &operator=(const BookmarkIconManager &&other) = delete;

	HIMAGELIST GetImageList();
	int GetBookmarkItemIconIndex(const BookmarkItem *bookmarkItem);
	void RemoveIcon(int iconIndex);

private:
	int GetIconForBookmark(const BookmarkItem *bookmark);
	int AddSystemIconToImageList(int systemIconIndex);

	IExplorerplusplus *m_expp;
	IconAvailableCallback m_callback;

	wil::unique_himagelist m_imageList;

	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	int m_defaultFolderIconSystemImageListIndex;
	int m_defaultFolderIconIndex;
	int m_bookmarkFolderIconIndex;
	IconFetcher *m_iconFetcher;

	std::shared_ptr<bool> m_destroyed;
};