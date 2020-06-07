// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "ItemData.h"
#include "ViewModes.h"
#include "../Helper/ShellHelper.h"
#include <boost/scope_exit.hpp>
#include <list>

#pragma warning(                                                                                   \
	disable : 4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

#define THUMBNAIL_TYPE_ICON 0
#define THUMBNAIL_TYPE_EXTRACTED 1

void ShellBrowser::SetupThumbnailsView()
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
	SHGetImageList(SHIL_LARGE, IID_PPV_ARGS(&pImageList));
	ListView_SetImageList(m_hListView, (HIMAGELIST) pImageList, LVSIL_NORMAL);
	pImageList->Release();

	m_hListViewImageList = ListView_GetImageList(m_hListView, LVSIL_NORMAL);

	ListView_SetExtendedListViewStyleEx(m_hListView, LVS_EX_BORDERSELECT, LVS_EX_BORDERSELECT);

	ListView_SetIconSpacing(m_hListView, THUMBNAIL_ITEM_HORIZONTAL_SPACING + THUMBNAIL_ITEM_WIDTH,
		THUMBNAIL_ITEM_VERTICAL_SPACING + THUMBNAIL_ITEM_HEIGHT);

	himl = ImageList_Create(
		THUMBNAIL_ITEM_WIDTH, THUMBNAIL_ITEM_HEIGHT, ILC_COLOR32, nItems, nItems + 100);
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

	/* Remove item borders. */
	ListView_SetExtendedListViewStyleEx(m_hListView, LVS_EX_BORDERSELECT, 0);

	/* Reset to the default icon spacing. */
	ListView_SetIconSpacing(m_hListView, -1, -1);

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

	auto result =
		m_thumbnailThreadPool.push([this, thumbnailResultID, internalIndex, basicItemInfo](int id) {
			UNREFERENCED_PARAMETER(id);

			return FindThumbnailAsync(m_hListView, thumbnailResultID, internalIndex, basicItemInfo);
		});

	m_thumbnailResults.insert({ thumbnailResultID, std::move(result) });
}

std::optional<ShellBrowser::ThumbnailResult_t> ShellBrowser::FindThumbnailAsync(
	HWND listView, int thumbnailResultId, int internalIndex, const BasicItemInfo_t &basicItemInfo)
{
	IShellFolder *pShellFolder = nullptr;
	HRESULT hr =
		SHBindToParent(basicItemInfo.pidlComplete.get(), IID_PPV_ARGS(&pShellFolder), nullptr);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	BOOST_SCOPE_EXIT(pShellFolder)
	{
		pShellFolder->Release();
	}
	BOOST_SCOPE_EXIT_END

	IExtractImage *pExtractImage = nullptr;
	auto pridl = basicItemInfo.pridl.get();
	hr = GetUIObjectOf(pShellFolder, nullptr, 1, const_cast<PCUITEMID_CHILD *>(&pridl),
		IID_PPV_ARGS(&pExtractImage));

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	BOOST_SCOPE_EXIT(pExtractImage)
	{
		pExtractImage->Release();
	}
	BOOST_SCOPE_EXIT_END

	SIZE size;
	size.cx = THUMBNAIL_ITEM_WIDTH;
	size.cy = THUMBNAIL_ITEM_HEIGHT;

	DWORD dwFlags = IEIFLAG_OFFLINE | IEIFLAG_QUALITY;

	// As per the MSDN documentation, GetLocation must be called before
	// Extract.
	TCHAR szImage[MAX_PATH];
	DWORD dwPriority;
	hr = pExtractImage->GetLocation(
		szImage, SIZEOF_ARRAY(szImage), &dwPriority, &size, 32, &dwFlags);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	wil::unique_hbitmap thumbnailBitmap;
	hr = pExtractImage->Extract(&thumbnailBitmap);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	PostMessage(listView, WM_APP_THUMBNAIL_RESULT_READY, thumbnailResultId, 0);

	ThumbnailResult_t result;
	result.itemInternalIndex = internalIndex;
	result.bitmap = std::move(thumbnailBitmap);

	return result;
}

void ShellBrowser::ProcessThumbnailResult(int thumbnailResultId)
{
	auto itr = m_thumbnailResults.find(thumbnailResultId);

	if (itr == m_thumbnailResults.end())
	{
		return;
	}

	if (m_folderSettings.viewMode != +ViewMode::Thumbnails)
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

int ShellBrowser::GetThumbnailInternal(
	int iType, int iInternalIndex, HBITMAP hThumbnailBitmap) const
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
	hBackingBitmap = CreateCompatibleBitmap(hdc, THUMBNAIL_ITEM_WIDTH, THUMBNAIL_ITEM_HEIGHT);
	hBackingBitmapOld = (HBITMAP) SelectObject(hdcBacking, hBackingBitmap);

	/* Set the background of the new bitmap to be the same color as the
	background in the listview. */
	hbr = CreateSolidBrush(ListView_GetBkColor(m_hListView));
	RECT rect = { 0, 0, THUMBNAIL_ITEM_WIDTH, THUMBNAIL_ITEM_HEIGHT };
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

	DrawIconEx(hdcBacking, (THUMBNAIL_ITEM_WIDTH - iIconWidth) / 2,
		(THUMBNAIL_ITEM_HEIGHT - iIconHeight) / 2, hIcon, 0, 0, 0, nullptr, DI_NORMAL);
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
	BitBlt(hdcBacking, (THUMBNAIL_ITEM_WIDTH - bm.bmWidth) / 2,
		(THUMBNAIL_ITEM_HEIGHT - bm.bmHeight) / 2, THUMBNAIL_ITEM_WIDTH, THUMBNAIL_ITEM_HEIGHT,
		hdcThumbnail, 0, 0, SRCCOPY);

	SelectObject(hdcThumbnail, hThumbnailBitmapOld);
	DeleteDC(hdcThumbnail);
}