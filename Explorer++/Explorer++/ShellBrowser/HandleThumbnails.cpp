// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "ItemData.h"
#include "ViewModes.h"
#include <wil/com.h>
#include <thumbcache.h>
#include <list>

#define THUMBNAIL_TYPE_ICON 0
#define THUMBNAIL_TYPE_EXTRACTED 1

void ShellBrowser::SetupThumbnailsView(int iImageList)
{
	HIMAGELIST himl;
	LVITEM lvItem;
	int nItems;
	int i = 0;

	nItems = ListView_GetItemCount(m_hListView);

	IImageList *pImageList = nullptr;

	/* Need to get the normal (32x32) image list for thumbnails, so that
	the regular size icon is shown for items with a thumbnail that hasn't
	been found yet (and not the large or extra large icon). */
	SHGetImageList(iImageList, IID_PPV_ARGS(&pImageList));
	ListView_SetImageList(m_hListView, (HIMAGELIST) pImageList, LVSIL_NORMAL);
	pImageList->Release();

	m_hListViewImageList = ListView_GetImageList(m_hListView, LVSIL_NORMAL);

	himl = ImageList_Create(GetThumbnailItemWidth(), GetThumbnailItemHeight(), ILC_COLOR32, nItems,
		nItems + 100);
	ListView_SetImageList(m_hListView, himl, LVSIL_NORMAL);

	for (i = 0; i < nItems; i++)
	{
		lvItem.mask = LVIF_IMAGE;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		lvItem.iImage = I_IMAGECALLBACK;
		ListView_SetItem(m_hListView, &lvItem);
	}

	m_bThumbnailsSetup = TRUE;
}

void ShellBrowser::RemoveThumbnailsView()
{
	LVITEM lvItem;
	HIMAGELIST himl;
	int nItems;
	int i = 0;

	nItems = ListView_GetItemCount(m_hListView);

	m_thumbnailThreadPool.clear_queue();
	m_thumbnailResults.clear();

	for (i = 0; i < nItems; i++)
	{
		lvItem.mask = LVIF_IMAGE;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		lvItem.iImage = I_IMAGECALLBACK;
		ListView_SetItem(m_hListView, &lvItem);
	}

	/* Destroy the thumbnails imagelist. */
	himl = ListView_GetImageList(m_hListView, LVSIL_NORMAL);

	ImageList_Destroy(himl);

	m_bThumbnailsSetup = FALSE;
}

void ShellBrowser::QueueThumbnailTask(int internalIndex)
{
	int thumbnailResultID = m_thumbnailResultIDCounter++;

	BasicItemInfo_t basicItemInfo = getBasicItemInfo(internalIndex);

	auto result = m_thumbnailThreadPool.push(
		[this, thumbnailResultID, internalIndex, basicItemInfo](
			int id) -> std::optional<ThumbnailResult_t>
		{
			UNREFERENCED_PARAMETER(id);

			auto bitmap = GetThumbnail(basicItemInfo.pidlComplete.get(),
				WTS_EXTRACT | WTS_SCALETOREQUESTEDSIZE);

			if (!bitmap)
			{
				return std::nullopt;
			}

			PostMessage(m_hListView, WM_APP_THUMBNAIL_RESULT_READY, thumbnailResultID, 0);

			ThumbnailResult_t result;
			result.itemInternalIndex = internalIndex;
			result.bitmap = std::move(bitmap);

			return result;
		});

	m_thumbnailResults.insert({ thumbnailResultID, std::move(result) });
}

std::optional<int> ShellBrowser::GetCachedThumbnailIndex(const ItemInfo_t &itemInfo)
{
	auto bitmap =
		GetThumbnail(itemInfo.pidlComplete.get(), WTS_INCACHEONLY | WTS_SCALETOREQUESTEDSIZE);

	if (!bitmap)
	{
		return std::nullopt;
	}

	return GetExtractedThumbnail(bitmap.get());
}

wil::unique_hbitmap ShellBrowser::GetThumbnail(PIDLIST_ABSOLUTE pidl, WTS_FLAGS flags)
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
	hr = thumbnailCache->GetThumbnail(shellItem.get(), GetThumbnailItemWidth(), flags,
		&sharedBitmap,
		nullptr, nullptr);

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

void ShellBrowser::ProcessThumbnailResult(int thumbnailResultId)
{
	auto itr = m_thumbnailResults.find(thumbnailResultId);

	if (itr == m_thumbnailResults.end())
	{
		return;
	}

	if (!IsViewModeThumbnail(m_folderSettings.viewMode))
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
	ListView_SetItem(m_hListView, &lvItem);
}

/* Draws a thumbnail based on an items icon. */
int ShellBrowser::GetIconThumbnail(int iInternalIndex) const
{
	return GetThumbnailInternal(THUMBNAIL_TYPE_ICON, iInternalIndex, nullptr);
}

/* Draws an items extracted thumbnail. */
int ShellBrowser::GetExtractedThumbnail(HBITMAP hThumbnailBitmap) const
{
	return GetThumbnailInternal(THUMBNAIL_TYPE_EXTRACTED, 0, hThumbnailBitmap);
}

int ShellBrowser::GetThumbnailInternal(int iType, int iInternalIndex,
	HBITMAP hThumbnailBitmap) const
{
	HDC hdc;
	HDC hdcBacking;
	HBITMAP hBackingBitmap;
	HBITMAP hBackingBitmapOld;
	HIMAGELIST himl;
	HBRUSH hbr;
	int iImage;

	hdc = GetDC(m_hListView);
	hdcBacking = CreateCompatibleDC(hdc);

	/* Backing bitmap. */
	hBackingBitmap =
		CreateCompatibleBitmap(hdc, GetThumbnailItemWidth(), GetThumbnailItemHeight());
	hBackingBitmapOld = (HBITMAP) SelectObject(hdcBacking, hBackingBitmap);

	/* Set the background of the new bitmap to be the same color as the
	background in the listview. */
	hbr = CreateSolidBrush(ListView_GetBkColor(m_hListView));
	RECT rect = { 0, 0, GetThumbnailItemWidth(), GetThumbnailItemHeight() };
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
	ReleaseDC(m_hListView, hdc);

	/* Add the new bitmap to the imagelist. */
	himl = ListView_GetImageList(m_hListView, LVSIL_NORMAL);
	iImage = ImageList_Add(himl, hBackingBitmap, nullptr);

	/* Now delete the backing bitmap. */
	DeleteObject(hBackingBitmap);

	return iImage;
}

void ShellBrowser::DrawIconThumbnailInternal(HDC hdcBacking, int iInternalIndex) const
{
	HICON hIcon;
	SHFILEINFO shfi;
	int iIconWidth;
	int iIconHeight;

	SHGetFileInfo((LPCTSTR) m_itemInfoMap.at(iInternalIndex).pidlComplete.get(), 0, &shfi,
		sizeof(shfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);

	hIcon = ImageList_GetIcon(m_hListViewImageList, shfi.iIcon, ILD_NORMAL);

	ImageList_GetIconSize(m_hListViewImageList, &iIconWidth, &iIconHeight);

	DrawIconEx(hdcBacking, (GetThumbnailItemWidth() - iIconWidth) / 2,
		(GetThumbnailItemHeight() - iIconHeight) / 2, hIcon, 0, 0, 0, nullptr, DI_NORMAL);
	DestroyIcon(hIcon);
}

void ShellBrowser::DrawThumbnailInternal(HDC hdcBacking, HBITMAP hThumbnailBitmap) const
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
	BitBlt(hdcBacking, (GetThumbnailItemWidth() - bm.bmWidth) / 2,
		(GetThumbnailItemHeight() - bm.bmHeight) / 2, GetThumbnailItemWidth(),
		GetThumbnailItemHeight(),
		hdcThumbnail, 0, 0, SRCCOPY);

	SelectObject(hdcThumbnail, hThumbnailBitmapOld);
	DeleteDC(hdcThumbnail);
}
