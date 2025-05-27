// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserImpl.h"
#include "ItemData.h"
#include "ViewModes.h"
#include <wil/com.h>
#include <thumbcache.h>
#include <list>

#define THUMBNAIL_TYPE_ICON 0
#define THUMBNAIL_TYPE_EXTRACTED 1

void ShellBrowserImpl::SetupThumbnailsView(int shellImageListType)
{
	// This will be used in cases where the thumbnail hasn't been retrieved yet and the standard
	// icon needs to be shown instead.
	IImageList *imageList = nullptr;
	FAIL_FAST_IF_FAILED(SHGetImageList(shellImageListType, IID_PPV_ARGS(&imageList)));
	m_directoryState.thumbnailsShellImageList = reinterpret_cast<HIMAGELIST>(imageList);

	int numItems = ListView_GetItemCount(m_listView);
	m_directoryState.thumbnailsImageList.reset(
		ImageList_Create(m_thumbnailItemWidth, m_thumbnailItemHeight, ILC_COLOR32, numItems, 10));
	ListView_SetImageList(m_listView, m_directoryState.thumbnailsImageList.get(), LVSIL_NORMAL);

	InvalidateAllItemImages();
}

void ShellBrowserImpl::RemoveThumbnailsView()
{
	m_thumbnailThreadPool.clear_queue();
	m_thumbnailResults.clear();

	InvalidateAllItemImages();

	ListView_SetImageList(m_listView, nullptr, LVSIL_NORMAL);

	m_directoryState.thumbnailsShellImageList = nullptr;
	m_directoryState.thumbnailsImageList.reset();
}

void ShellBrowserImpl::InvalidateAllItemImages()
{
	int numItems = ListView_GetItemCount(m_listView);

	for (int i = 0; i < numItems; i++)
	{
		LVITEM lvItem;
		lvItem.mask = LVIF_IMAGE;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		lvItem.iImage = I_IMAGECALLBACK;
		ListView_SetItem(m_listView, &lvItem);
	}
}

void ShellBrowserImpl::QueueThumbnailTask(int internalIndex)
{
	int thumbnailResultID = m_thumbnailResultIDCounter++;

	BasicItemInfo_t basicItemInfo = getBasicItemInfo(internalIndex);

	auto result = m_thumbnailThreadPool.push(
		[listView = m_listView, thumbnailResultID, internalIndex, basicItemInfo,
			thumbnailSize = m_thumbnailItemWidth](int id) -> std::optional<ThumbnailResult_t>
		{
			UNREFERENCED_PARAMETER(id);

			auto bitmap = GetThumbnail(basicItemInfo.pidlComplete.get(), thumbnailSize,
				WTS_EXTRACT | WTS_SCALETOREQUESTEDSIZE);

			if (!bitmap)
			{
				return std::nullopt;
			}

			PostMessage(listView, WM_APP_THUMBNAIL_RESULT_READY, thumbnailResultID, 0);

			ThumbnailResult_t result;
			result.itemInternalIndex = internalIndex;
			result.bitmap = std::move(bitmap);

			return result;
		});

	m_thumbnailResults.insert({ thumbnailResultID, std::move(result) });
}

std::optional<int> ShellBrowserImpl::GetCachedThumbnailIndex(const ItemInfo_t &itemInfo)
{
	auto bitmap = GetThumbnail(itemInfo.pidlComplete.Raw(), m_thumbnailItemWidth,
		WTS_INCACHEONLY | WTS_SCALETOREQUESTEDSIZE);

	if (!bitmap)
	{
		return std::nullopt;
	}

	return GetExtractedThumbnail(bitmap.get());
}

wil::unique_hbitmap ShellBrowserImpl::GetThumbnail(PCIDLIST_ABSOLUTE pidl, UINT thumbnailSize,
	WTS_FLAGS flags)
{
	wil::com_ptr_nothrow<IShellItem> shellItem;
	HRESULT hr = SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&shellItem));

	if (FAILED(hr))
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<IThumbnailCache> thumbnailCache;
	hr = CoCreateInstance(CLSID_LocalThumbnailCache, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&thumbnailCache));

	if (FAILED(hr))
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<ISharedBitmap> sharedBitmap;
	hr = thumbnailCache->GetThumbnail(shellItem.get(), thumbnailSize, flags, &sharedBitmap, nullptr,
		nullptr);

	if (FAILED(hr))
	{
		return nullptr;
	}

	HBITMAP bitmap;
	hr = sharedBitmap->GetSharedBitmap(&bitmap);

	if (FAILED(hr))
	{
		return nullptr;
	}

	// Note that the bitmap is copied here, since it's owned by the ISharedBitmap instance. As soon
	// as that instance is destroyed, the bitmap will be destroyed.
	return wil::unique_hbitmap(
		reinterpret_cast<HBITMAP>(CopyImage(bitmap, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR)));
}

void ShellBrowserImpl::ProcessThumbnailResult(int thumbnailResultId)
{
	auto itr = m_thumbnailResults.find(thumbnailResultId);

	if (itr == m_thumbnailResults.end())
	{
		return;
	}

	if (!IsThumbnailsViewMode(m_folderSettings.viewMode))
	{
		return;
	}

	auto result = itr->second.get();

	if (!result)
	{
		// Thumbnail lookup failed.
		return;
	}

	int imageIndex = GetExtractedThumbnail(result->bitmap.get());

	auto index = LocateItemByInternalIndex(result->itemInternalIndex);

	if (!index)
	{
		return;
	}

	LVITEM lvItem;
	lvItem.mask = LVIF_IMAGE;
	lvItem.iItem = *index;
	lvItem.iSubItem = 0;
	lvItem.iImage = imageIndex;
	ListView_SetItem(m_listView, &lvItem);
}

/* Draws a thumbnail based on an items icon. */
int ShellBrowserImpl::GetIconThumbnail(int iInternalIndex) const
{
	return GetThumbnailInternal(THUMBNAIL_TYPE_ICON, iInternalIndex, nullptr);
}

/* Draws an items extracted thumbnail. */
int ShellBrowserImpl::GetExtractedThumbnail(HBITMAP hThumbnailBitmap) const
{
	return GetThumbnailInternal(THUMBNAIL_TYPE_EXTRACTED, 0, hThumbnailBitmap);
}

int ShellBrowserImpl::GetThumbnailInternal(int iType, int iInternalIndex,
	HBITMAP hThumbnailBitmap) const
{
	HDC hdc;
	HDC hdcBacking;
	HBITMAP hBackingBitmap;
	HBITMAP hBackingBitmapOld;
	HIMAGELIST himl;
	HBRUSH hbr;
	int iImage;

	hdc = GetDC(m_listView);
	hdcBacking = CreateCompatibleDC(hdc);

	/* Backing bitmap. */
	hBackingBitmap = CreateCompatibleBitmap(hdc, m_thumbnailItemWidth, m_thumbnailItemHeight);
	hBackingBitmapOld = (HBITMAP) SelectObject(hdcBacking, hBackingBitmap);

	/* Set the background of the new bitmap to be the same color as the
	background in the listview. */
	hbr = CreateSolidBrush(ListView_GetBkColor(m_listView));
	RECT rect = { 0, 0, m_thumbnailItemWidth, m_thumbnailItemHeight };
	FillRect(hdcBacking, &rect, hbr);

	if (iType == THUMBNAIL_TYPE_ICON)
	{
		DrawIconThumbnailInternal(hdcBacking, iInternalIndex);
	}
	else if (iType == THUMBNAIL_TYPE_EXTRACTED)
	{
		DrawThumbnailInternal(hdcBacking, hThumbnailBitmap);
	}

	/* Clean up...
	Everything EXCEPT the backing bitmap should be
	deleted here (at the very least, the backing
	bitmap needs to be selected out of its DC). */
	SelectObject(hdcBacking, hBackingBitmapOld);
	DeleteDC(hdcBacking);
	ReleaseDC(m_listView, hdc);

	/* Add the new bitmap to the imagelist. */
	himl = ListView_GetImageList(m_listView, LVSIL_NORMAL);
	iImage = ImageList_Add(himl, hBackingBitmap, nullptr);

	/* Now delete the backing bitmap. */
	DeleteObject(hBackingBitmap);

	return iImage;
}

void ShellBrowserImpl::DrawIconThumbnailInternal(HDC hdcBacking, int iInternalIndex) const
{
	HICON hIcon;
	SHFILEINFO shfi;
	int iIconWidth;
	int iIconHeight;

	SHGetFileInfo((LPCTSTR) m_itemInfoMap.at(iInternalIndex).pidlComplete.Raw(), 0, &shfi,
		sizeof(shfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);

	hIcon = ImageList_GetIcon(m_directoryState.thumbnailsShellImageList, shfi.iIcon, ILD_NORMAL);

	ImageList_GetIconSize(m_directoryState.thumbnailsShellImageList, &iIconWidth, &iIconHeight);

	DrawIconEx(hdcBacking, (m_thumbnailItemWidth - iIconWidth) / 2,
		(m_thumbnailItemHeight - iIconHeight) / 2, hIcon, 0, 0, 0, nullptr, DI_NORMAL);
	DestroyIcon(hIcon);
}

void ShellBrowserImpl::DrawThumbnailInternal(HDC hdcBacking, HBITMAP hThumbnailBitmap) const
{
	HDC hdcThumbnail;
	HBITMAP hThumbnailBitmapOld;
	BITMAP bm;

	/* Thumbnail bitmap. */
	hdcThumbnail = CreateCompatibleDC(hdcBacking);
	hThumbnailBitmapOld = (HBITMAP) SelectObject(hdcThumbnail, hThumbnailBitmap);

	GetObject(hThumbnailBitmap, sizeof(BITMAP), &bm);

	/* Now, draw the thumbnail bitmap (in its centered position)
	directly on top of the new bitmap. */
	BitBlt(hdcBacking, (m_thumbnailItemWidth - bm.bmWidth) / 2,
		(m_thumbnailItemHeight - bm.bmHeight) / 2, m_thumbnailItemWidth, m_thumbnailItemHeight,
		hdcThumbnail, 0, 0, SRCCOPY);

	SelectObject(hdcThumbnail, hThumbnailBitmapOld);
	DeleteDC(hdcThumbnail);
}
