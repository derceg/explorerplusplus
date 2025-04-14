// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkItem.h"
#include "Icon.h"
#include "IconFetcher.h"
#include "ResourceLoader.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WeakPtr.h"

BookmarkIconManager::BookmarkIconManager(const ResourceLoader *resourceLoader,
	IconFetcher *iconFetcher, int iconWidth, int iconHeight) :
	m_iconFetcher(iconFetcher),
	m_weakPtrFactory(this)
{
	m_imageList.reset(ImageList_Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 0, 1));

	wil::unique_hbitmap folderIcon =
		resourceLoader->LoadBitmapFromPNGAndScale(Icon::Folder, iconWidth, iconHeight);
	m_bookmarkFolderIconIndex = ImageList_Add(m_imageList.get(), folderIcon.get(), nullptr);

	SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList));

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
	IconAvailableCallback callback)
{
	int iconIndex;

	if (bookmarkItem->IsFolder())
	{
		iconIndex = m_bookmarkFolderIconIndex;
	}
	else
	{
		iconIndex = GetIconForBookmark(bookmarkItem, callback);
	}

	return iconIndex;
}

int BookmarkIconManager::GetIconForBookmark(const BookmarkItem *bookmark,
	IconAvailableCallback callback)
{
	int iconIndex = m_defaultFolderIconIndex;

	auto cachedIconIndex = m_iconFetcher->GetCachedIconIndex(bookmark->GetLocation());

	if (cachedIconIndex)
	{
		iconIndex = AddSystemIconToImageList(*cachedIconIndex);
	}
	else
	{
		m_iconFetcher->QueueIconTask(bookmark->GetLocation(),
			[callback, self = m_weakPtrFactory.GetWeakPtr()](int iconIndex, int overlayIndex)
			{
				UNREFERENCED_PARAMETER(overlayIndex);

				if (!self || !callback)
				{
					return;
				}

				if (iconIndex == self->m_defaultFolderIconSystemImageListIndex)
				{
					// Bookmarks use the standard folder icon by default, so if that's the icon
					// they're actually using, nothing else needs to happen.
					return;
				}

				int copiedIconIndex = self->AddSystemIconToImageList(iconIndex);
				callback(copiedIconIndex);
			});
	}

	return iconIndex;
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
