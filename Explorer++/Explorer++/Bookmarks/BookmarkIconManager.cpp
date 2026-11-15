// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "AsyncIconFetcher.h"
#include "Bookmarks/BookmarkItem.h"
#include "Icon.h"
#include "ResourceLoader.h"
#include "RuntimeHelper.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WeakPtr.h"

BookmarkIconManager::BookmarkIconManager(const Runtime *runtime, AsyncIconFetcher *iconFetcher,
	const ResourceLoader *resourceLoader, int iconWidth, int iconHeight) :
	m_runtime(runtime),
	m_iconFetcher(iconFetcher)
{
	m_imageList.reset(ImageList_Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 0, 1));

	auto folderIcon =
		resourceLoader->LoadBitmap(Icon::Folder, iconWidth, iconHeight);
	m_bookmarkFolderIconIndex =
		ImageHelper::AddBitmapToImageList(m_imageList.get(), folderIcon.get());

	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList)));

	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(m_defaultFolderIconSystemImageListIndex));
	m_defaultFolderIconIndex = ImageHelper::CopyImageListIcon(m_imageList.get(),
		reinterpret_cast<HIMAGELIST>(m_systemImageList.get()),
		m_defaultFolderIconSystemImageListIndex);
}

HIMAGELIST BookmarkIconManager::GetImageList()
{
	return m_imageList.get();
}

int BookmarkIconManager::GetBookmarkItemIconIndex(const BookmarkItem *bookmarkItem,
	IconAvailableCallback callback, std::stop_token stopToken)
{
	if (bookmarkItem->IsFolder())
	{
		return GetBookmarkFolderIconIndex();
	}
	else
	{
		return GetBookmarkIconIndex(bookmarkItem, std::move(callback), stopToken);
	}
}

int BookmarkIconManager::GetBookmarkFolderIconIndex()
{
	return m_bookmarkFolderIconIndex;
}

int BookmarkIconManager::GetBookmarkIconIndex(const BookmarkItem *bookmark,
	IconAvailableCallback callback, std::stop_token stopToken)
{
	CHECK(bookmark->IsBookmark());

	int iconIndex = m_defaultFolderIconIndex;
	auto cachedIconIndex = m_iconFetcher->MaybeGetCachedIconIndex(bookmark->GetLocation());

	if (cachedIconIndex)
	{
		iconIndex = AddSystemIconToImageList(*cachedIconIndex);
	}
	else
	{
		RetrieveIconForBookmark(m_weakPtrFactory.GetWeakPtr(), bookmark->GetLocation(),
			std::move(callback), stopToken);
	}

	return iconIndex;
}

concurrencpp::null_result BookmarkIconManager::RetrieveIconForBookmark(
	WeakPtr<BookmarkIconManager> weakSelf, const std::wstring bookmarkLocation,
	IconAvailableCallback callback, std::stop_token stopToken)
{
	auto *runtime = weakSelf->m_runtime;
	auto iconFetcher = weakSelf->m_iconFetcher;

	auto iconInfo = co_await iconFetcher->GetIconIndexAsync(bookmarkLocation, stopToken);

	if (!iconInfo)
	{
		co_return;
	}

	co_await ResumeOnUiThread(runtime);

	if (stopToken.stop_requested() || !weakSelf)
	{
		co_return;
	}

	if (iconInfo->iconIndex == weakSelf->m_defaultFolderIconSystemImageListIndex)
	{
		// Bookmarks use the standard folder icon by default, so if that's the icon they're actually
		// using, nothing else needs to happen.
		co_return;
	}

	int copiedIconIndex = weakSelf->AddSystemIconToImageList(iconInfo->iconIndex);
	callback(copiedIconIndex);
}

int BookmarkIconManager::AddSystemIconToImageList(int systemIconIndex)
{
	// Note that while it would be possible to prevent other icons from being inserted into the
	// image list more than once, it would be a little more complicated (the number of uses for an
	// icon would have to be tracked) and there would be more room for things to go wrong.
	// It's likely not worth it, since most folders will probably use the default icon.
	if (systemIconIndex == m_defaultFolderIconSystemImageListIndex)
	{
		return m_defaultFolderIconIndex;
	}

	return ImageHelper::CopyImageListIcon(m_imageList.get(),
		reinterpret_cast<HIMAGELIST>(m_systemImageList.get()), systemIconIndex);
}
