/******************************************************************
 *
 * Project: ShellBrowser
 * File: HandleThumbnails.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Handles thumbnails view, including
 * preparing the listview and drawing
 * image thumbnails.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "iShellView.h"
#include "iShellBrowser_internal.h"
#include "ViewModes.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"
#include <boost/scope_exit.hpp>
#include <list>

#pragma warning(disable:4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

void CShellBrowser::SetupThumbnailsView(void)
{
	HIMAGELIST himl;
	LVITEM lvItem;
	int nItems;
	int i = 0;

	nItems = ListView_GetItemCount(m_hListView);

	IImageList *pImageList = NULL;

	/* Need to get the normal (32x32) image list for thumbnails, so that
	the regular size icon is shown for items with a thumbnail that hasn't
	been found yet (and not the large or extra large icon). */
	SHGetImageList(SHIL_LARGE, IID_PPV_ARGS(&pImageList));
	ListView_SetImageList(m_hListView,(HIMAGELIST)pImageList,LVSIL_NORMAL);
	pImageList->Release();

	m_hListViewImageList = ListView_GetImageList(m_hListView,LVSIL_NORMAL);

	ListView_SetExtendedListViewStyleEx(m_hListView,LVS_EX_BORDERSELECT,LVS_EX_BORDERSELECT);

	ListView_SetIconSpacing(m_hListView,THUMBNAIL_ITEM_HORIZONTAL_SPACING + THUMBNAIL_ITEM_WIDTH,
		THUMBNAIL_ITEM_VERTICAL_SPACING + THUMBNAIL_ITEM_HEIGHT);

	himl = ImageList_Create(THUMBNAIL_ITEM_WIDTH,THUMBNAIL_ITEM_HEIGHT,ILC_COLOR32,nItems,nItems + 100);
	ListView_SetImageList(m_hListView,himl,LVSIL_NORMAL);

	for(i = 0;i < nItems;i++)
	{
		lvItem.mask		= LVIF_IMAGE;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		lvItem.iImage	= I_IMAGECALLBACK;
		ListView_SetItem(m_hListView,&lvItem);
	}

	m_bThumbnailsSetup = TRUE;
}

void CShellBrowser::RemoveThumbnailsView(void)
{
	LVITEM		lvItem;
	HIMAGELIST	himl;
	int			nItems;
	int			i = 0;

	/* Remove item borders. */
	ListView_SetExtendedListViewStyleEx(m_hListView,LVS_EX_BORDERSELECT,0);

	/* Reset to the default icon spacing. */
	ListView_SetIconSpacing(m_hListView,-1,-1);

	nItems = ListView_GetItemCount(m_hListView);

	m_itemImageThreadPool.clear_queue();
	m_thumbnailResults.clear();

	for(i = 0;i < nItems;i++)
	{
		lvItem.mask		= LVIF_IMAGE;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		lvItem.iImage	= I_IMAGECALLBACK;
		ListView_SetItem(m_hListView,&lvItem);
	}

	/* Destroy the thumbnails imagelist. */
	himl = ListView_GetImageList(m_hListView,LVSIL_NORMAL);

	ImageList_Destroy(himl);

	m_bThumbnailsSetup = FALSE;
}

void CShellBrowser::QueueThumbnailTask(int internalIndex)
{
	int thumbnailResultID = m_thumbnailResultIDCounter++;

	auto result = m_itemImageThreadPool.push([this, thumbnailResultID, internalIndex](int id) {
		UNREFERENCED_PARAMETER(id);

		return this->FindThumbnailAsync(thumbnailResultID, internalIndex);
	});

	m_thumbnailResults.insert({ thumbnailResultID, std::move(result) });
}

boost::optional<CShellBrowser::ImageResult_t> CShellBrowser::FindThumbnailAsync(int thumbnailResultId, int internalIndex) const
{
	IShellFolder *pShellFolder = nullptr;
	HRESULT hr = BindToIdl(m_pidlDirectory, IID_PPV_ARGS(&pShellFolder));

	if (FAILED(hr))
	{
		return boost::none;
	}

	BOOST_SCOPE_EXIT(pShellFolder) {
		pShellFolder->Release();
	} BOOST_SCOPE_EXIT_END

	IExtractImage *pExtractImage = nullptr;
	LPCITEMIDLIST pridl = m_extraItemInfoMap.at(internalIndex).pridl.get();
	hr = GetUIObjectOf(pShellFolder, NULL, 1, &pridl, IID_PPV_ARGS(&pExtractImage));

	if (FAILED(hr))
	{
		return boost::none;
	}

	BOOST_SCOPE_EXIT(pExtractImage) {
		pExtractImage->Release();
	} BOOST_SCOPE_EXIT_END

	SIZE size;
	size.cx = THUMBNAIL_ITEM_WIDTH;
	size.cy = THUMBNAIL_ITEM_HEIGHT;

	DWORD dwFlags = IEIFLAG_OFFLINE | IEIFLAG_QUALITY;

	// As per the MSDN documentation, GetLocation must be called before
	// Extract.
	TCHAR szImage[MAX_PATH];
	DWORD dwPriority;
	hr = pExtractImage->GetLocation(szImage, SIZEOF_ARRAY(szImage), &dwPriority, &size, 32, &dwFlags);

	if (FAILED(hr))
	{
		return boost::none;
	}

	HBITMAP hThumbnailBitmap;
	hr = pExtractImage->Extract(&hThumbnailBitmap);

	if (FAILED(hr))
	{
		return boost::none;
	}

	BOOST_SCOPE_EXIT(hThumbnailBitmap) {
		DeleteObject(hThumbnailBitmap);
	} BOOST_SCOPE_EXIT_END

	int imageIndex = GetExtractedThumbnail(hThumbnailBitmap);

	PostMessage(m_hListView, WM_APP_THUMBNAIL_RESULT_READY, thumbnailResultId, 0);

	ImageResult_t result;
	result.itemInternalIndex = internalIndex;
	result.iconIndex = imageIndex;

	return result;
}

void CShellBrowser::ProcessThumbnailResult(int thumbnailResultId)
{
	auto itr = m_thumbnailResults.find(thumbnailResultId);

	if (itr == m_thumbnailResults.end())
	{
		return;
	}

	if (m_ViewMode != VM_THUMBNAILS)
	{
		return;
	}

	auto result = itr->second.get();

	if (!result)
	{
		// Thumbnail lookup failed.
		return;
	}

	auto index = LocateItemByInternalIndex(result->itemInternalIndex);

	if (!index)
	{
		return;
	}

	LVITEM lvItem;
	lvItem.mask = LVIF_IMAGE;
	lvItem.iItem = *index;
	lvItem.iSubItem = 0;
	lvItem.iImage = result->iconIndex;
	ListView_SetItem(m_hListView, &lvItem);
}

/* Draws a thumbnail based on an items icon. */
int CShellBrowser::GetIconThumbnail(int iInternalIndex) const
{
	return GetThumbnailInternal(THUMBNAIL_TYPE_ICON,iInternalIndex,
		NULL);
}

/* Draws an items extracted thumbnail. */
int CShellBrowser::GetExtractedThumbnail(HBITMAP hThumbnailBitmap) const
{
	return GetThumbnailInternal(THUMBNAIL_TYPE_EXTRACTED,0,
		hThumbnailBitmap);
}

int CShellBrowser::GetThumbnailInternal(int iType,
int iInternalIndex,HBITMAP hThumbnailBitmap) const
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
	hBackingBitmap = CreateCompatibleBitmap(hdc,THUMBNAIL_ITEM_WIDTH,THUMBNAIL_ITEM_HEIGHT);
	hBackingBitmapOld = (HBITMAP)SelectObject(hdcBacking,hBackingBitmap);

	/* Set the background of the new bitmap to be the same color as the
	background in the listview. */
	hbr = CreateSolidBrush(ListView_GetBkColor(m_hListView));
	RECT rect = {0,0,THUMBNAIL_ITEM_WIDTH,THUMBNAIL_ITEM_HEIGHT};
	FillRect(hdcBacking,&rect,hbr);

	if(iType == THUMBNAIL_TYPE_ICON)
		DrawIconThumbnailInternal(hdcBacking,iInternalIndex);
	else if(iType == THUMBNAIL_TYPE_EXTRACTED)
		DrawThumbnailInternal(hdcBacking,hThumbnailBitmap);

	/* Clean up...
	Everything EXCEPT the backing bitmap should be
	deleted here (at the very least, the backing
	bitmap needs to be selected out of its DC). */
	SelectObject(hdcBacking,hBackingBitmapOld);
	DeleteDC(hdcBacking);
	ReleaseDC(m_hListView,hdc);

	/* Add the new bitmap to the imagelist. */
	himl = ListView_GetImageList(m_hListView,LVSIL_NORMAL);
	iImage = ImageList_Add(himl,hBackingBitmap,NULL);

	/* Now delete the backing bitmap. */
	DeleteObject(hBackingBitmap);

	return iImage;
}

void CShellBrowser::DrawIconThumbnailInternal(HDC hdcBacking,int iInternalIndex) const
{
	LPITEMIDLIST pidlFull = NULL;
	HICON hIcon;
	SHFILEINFO shfi;
	int iIconWidth;
	int iIconHeight;

	pidlFull = ILCombine(m_pidlDirectory,m_extraItemInfoMap.at(iInternalIndex).pridl.get());
	SHGetFileInfo((LPCTSTR)pidlFull,0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_SYSICONINDEX);

	hIcon = ImageList_GetIcon(m_hListViewImageList,
		shfi.iIcon,ILD_NORMAL);

	ImageList_GetIconSize(m_hListViewImageList,&iIconWidth,&iIconHeight);

	DrawIconEx(hdcBacking,(THUMBNAIL_ITEM_WIDTH - iIconWidth) / 2,
		(THUMBNAIL_ITEM_HEIGHT - iIconHeight) / 2,hIcon,0,0,
		NULL,NULL,DI_NORMAL);
	DestroyIcon(hIcon);
}

void CShellBrowser::DrawThumbnailInternal(HDC hdcBacking,HBITMAP hThumbnailBitmap) const
{
	HDC hdcThumbnail;
	HBITMAP hThumbnailBitmapOld;
	BITMAP bm;

	/* Thumbnail bitmap. */
	hdcThumbnail = CreateCompatibleDC(hdcBacking);
	hThumbnailBitmapOld = (HBITMAP)SelectObject(hdcThumbnail,hThumbnailBitmap);

	GetObject(hThumbnailBitmap,sizeof(BITMAP),&bm);

	/* Now, draw the thumbnail bitmap (in its centered position)
	directly on top of the new bitmap. */
	BitBlt(hdcBacking,(THUMBNAIL_ITEM_WIDTH - bm.bmWidth) / 2,
		(THUMBNAIL_ITEM_HEIGHT - bm.bmHeight) / 2,
		THUMBNAIL_ITEM_WIDTH,THUMBNAIL_ITEM_HEIGHT,
		hdcThumbnail,0,0,SRCCOPY);

	SelectObject(hdcThumbnail,hThumbnailBitmapOld);
	DeleteDC(hdcThumbnail);
}