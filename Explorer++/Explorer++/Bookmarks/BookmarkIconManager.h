// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/WeakPtrFactory.h"
#include <concurrencpp/concurrencpp.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <stop_token>

class AsyncIconFetcher;
class BookmarkItem;
class ResourceLoader;
class Runtime;

class BookmarkIconManager
{
public:
	using IconAvailableCallback = std::function<void(int iconIndex)>;

	BookmarkIconManager(const Runtime *runtime, AsyncIconFetcher *iconFetcher,
		const ResourceLoader *resourceLoader, int iconWidth, int iconHeight);

	BookmarkIconManager(const BookmarkIconManager &other) = delete;
	BookmarkIconManager(const BookmarkIconManager &&other) = delete;
	BookmarkIconManager &operator=(const BookmarkIconManager &other) = delete;
	BookmarkIconManager &operator=(const BookmarkIconManager &&other) = delete;

	HIMAGELIST GetImageList();

	int GetBookmarkItemIconIndex(const BookmarkItem *bookmarkItem, IconAvailableCallback callback,
		std::stop_token stopToken);
	int GetBookmarkFolderIconIndex();
	int GetBookmarkIconIndex(const BookmarkItem *bookmark, IconAvailableCallback callback,
		std::stop_token stopToken);

private:
	static concurrencpp::null_result RetrieveIconForBookmark(WeakPtr<BookmarkIconManager> weakSelf,
		const std::wstring bookmarkLocation, IconAvailableCallback callback,
		std::stop_token stopToken);
	int AddSystemIconToImageList(int systemIconIndex);

	const Runtime *const m_runtime;
	AsyncIconFetcher *const m_iconFetcher;
	wil::unique_himagelist m_imageList;
	wil::com_ptr_nothrow<IImageList> m_systemImageList;
	int m_defaultFolderIconSystemImageListIndex;
	int m_defaultFolderIconIndex;
	int m_bookmarkFolderIconIndex;

	WeakPtrFactory<BookmarkIconManager> m_weakPtrFactory{ this };
};
