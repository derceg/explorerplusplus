/******************************************************************
 *
 * Project: ShellBrowser
 * File: HandleThumbnails.cpp
 * License: GPL - See COPYING in the top level directory
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
#include <list>
#include "IShellView.h"
#include "iShellBrowser_internal.h"
#include "../Helper/Controls.h"
#include "../Helper/Helper.h"
#include "../Helper/FileOperations.h"
#include "../Helper/FolderSize.h"


BOOL RemoveFromThumbnailsFinderQueue(ListViewInfo_t *pListViewInfo,HANDLE hStopEvent);

list<ListViewInfo_t>	g_ThumbnailQueue;
CRITICAL_SECTION		g_csThumbnails;
BOOL					g_bcsThumbnailInitialized = FALSE;

int g_nThumbnailAPCsQueued = 0;
int g_nThumbnailAPCsRun = 0;

void CFolderView::SetupThumbnailsView(void)
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
	SHGetImageList(SHIL_LARGE,IID_IImageList,(void **)&pImageList);
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

void CFolderView::RemoveThumbnailsView(void)
{
	LVITEM		lvItem;
	HIMAGELIST	himl;
	int			nItems;
	int			i = 0;

	/* Remove item borders. */
	ListView_SetExtendedListViewStyleEx(m_hListView,LVS_EX_BORDERSELECT,0);

	/* Reset to the default icon spacing. */
	ListView_SetIconSpacing(m_hListView,-1,-1);

	/* Destroy the thumbnails imagelist. */
	himl = ListView_GetImageList(m_hListView,LVSIL_NORMAL);

	ImageList_Destroy(himl);

	nItems = ListView_GetItemCount(m_hListView);

	EmptyThumbnailsQueue();

	for(i = 0;i < nItems;i++)
	{
		lvItem.mask		= LVIF_IMAGE;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		lvItem.iImage	= I_IMAGECALLBACK;
		ListView_SetItem(m_hListView,&lvItem);
	}

	m_bThumbnailsSetup = FALSE;
}

void CFolderView::EmptyThumbnailsQueue(void)
{
	EnterCriticalSection(&g_csThumbnails);
	g_ThumbnailQueue.empty();
	LeaveCriticalSection(&g_csThumbnails);
}

void CFolderView::AddToThumbnailFinderQueue(LPARAM lParam)
{
	EnterCriticalSection(&g_csThumbnails);

	ListViewInfo_t lvil;

	lvil.hListView	= m_hListView;
	lvil.iItem		= (int)lParam;
	lvil.pidlFull	= ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lParam].pridl);
	lvil.m_pExtraItemInfo	= &m_pExtraItemInfo[(int)lParam];
	lvil.hEvent		= m_hIconEvent;

	g_ThumbnailQueue.push_back(lvil);

	if(g_nThumbnailAPCsRun == g_nThumbnailAPCsQueued)
	{
		g_nThumbnailAPCsQueued++;

		QueueUserAPC(FindThumbnailAPC,m_hThread,(ULONG_PTR)this);
	}

	LeaveCriticalSection(&g_csThumbnails);
}

void CFolderView::SetThumbnailFlag(CItemObject *m_pExtraItemInfo)
{
	EnterCriticalSection(&g_csThumbnails);
	m_pExtraItemInfo->bThumbnailRetreived = TRUE;
	LeaveCriticalSection(&g_csThumbnails);
}

BOOL RemoveFromThumbnailsFinderQueue(ListViewInfo_t *pListViewInfo,HANDLE hStopEvent)
{
	BOOL bQueueNotEmpty;

	EnterCriticalSection(&g_csThumbnails);

	if(g_ThumbnailQueue.empty() == TRUE)
	{
		bQueueNotEmpty = FALSE;

		g_nThumbnailAPCsRun++;
	}
	else
	{
		list<ListViewInfo_t>::iterator itr;

		itr = g_ThumbnailQueue.end();

		itr--;

		*pListViewInfo = *itr;

		g_ThumbnailQueue.erase(itr);

		bQueueNotEmpty = TRUE;
	}

	LeaveCriticalSection(&g_csThumbnails);

	return bQueueNotEmpty;
}

/*
This creates the thumbnail bitmap for a file,
adds it to the current image list, and then sets
the index of the new image as the imaage for the
specified file.
Bitmap creation occurs as follows:
1. Thumbnail bitmap is extracted.
2. A new bitmap is created with the same width
   and height as the thumbnail box, and has its
   background set to the same color as the
   listview background.
3. The thumbnail bitmap is drawn on top of the
   new bitmap in a centered position.
4. The combined bitmap is then added to the
   imagelist for the listview.
5. The original item is found using its lParam
   value.
6. The image index for the new item is set to
   be the index of the combined bitmap in the
   imagelist.
*/
void CALLBACK FindThumbnailAPC(ULONG_PTR dwParam)
{
	IExtractImage *pExtractImage = NULL;
	IShellFolder *pShellDesktop = NULL;
	IShellFolder *pShellFolder = NULL;
	LPITEMIDLIST pidlParent = NULL;
	LPITEMIDLIST pridl = NULL;
	HBITMAP hThumbnailBitmap;
	SIZE size;
	TCHAR szImage[MAX_PATH];
	DWORD dwPriority;
	DWORD dwFlags;
	HRESULT hr;
	BOOL bQueueNotEmpty;
	ListViewInfo_t	pListViewInfo;
	CFolderView *pFolderView = NULL;

	pFolderView = reinterpret_cast<CFolderView *>(dwParam);

	/* If this module is in the process of been
	shut down, DO NOT load any more thumbnails. */
	if(pFolderView->GetTerminationStatus())
		return;

	bQueueNotEmpty = RemoveFromThumbnailsFinderQueue(&pListViewInfo,NULL);

	while(bQueueNotEmpty)
	{
		pidlParent = ILClone(pListViewInfo.pidlFull);
		ILRemoveLastID(pidlParent);

		pridl = ILClone(ILFindLastID(pListViewInfo.pidlFull));

		hr = SHGetDesktopFolder(&pShellDesktop);

		if(SUCCEEDED(hr))
		{
			hr = pShellDesktop->BindToObject(pidlParent,NULL,IID_IShellFolder,(void **)&pShellFolder);

			if(SUCCEEDED(hr))
			{
				hr = pShellFolder->GetUIObjectOf(NULL,1,(LPCITEMIDLIST *)&pridl,
					IID_IExtractImage,NULL,(void **)&pExtractImage);

				if(SUCCEEDED(hr))
				{
					dwFlags = IEIFLAG_OFFLINE|IEIFLAG_QUALITY;
					size.cx = THUMBNAIL_ITEM_WIDTH;
					size.cy = THUMBNAIL_ITEM_HEIGHT;

					/* Note that this may return E_PENDING (on Vista),
					which seems to indicate the request in pending.
					Attempting to extract the image appears to succeed
					regardless (perhaps after some delay). */
					pExtractImage->GetLocation(szImage,MAX_PATH,
						&dwPriority,&size,32,&dwFlags);

					hr = pExtractImage->Extract(&hThumbnailBitmap);

					if(SUCCEEDED(hr))
					{
						LVFINDINFO lvfi;
						LVITEM lvItem;
						int iItem;
						int iImage;

						iImage = pFolderView->GetExtractedThumbnail(hThumbnailBitmap);

						lvfi.flags	= LVFI_PARAM;
						lvfi.lParam	= pListViewInfo.iItem;
						iItem = ListView_FindItem(pListViewInfo.hListView,-1,&lvfi);

						/* If the item is still in the listview, set its
						image to the new image index. */
						if(iItem != -1)
						{
							lvItem.mask		= LVIF_IMAGE;
							lvItem.iItem	= iItem;
							lvItem.iSubItem	= 0;
							lvItem.iImage	= iImage;
							ListView_SetItem(pListViewInfo.hListView,&lvItem);

							pListViewInfo.m_pExtraItemInfo->bThumbnailRetreived = TRUE;
						}

						DeleteObject(hThumbnailBitmap);
					}

					pExtractImage->Release();
				}

				pShellFolder->Release();
			}

			pShellDesktop->Release();
		}

		CoTaskMemFree(pidlParent);
		CoTaskMemFree(pridl);

		bQueueNotEmpty = RemoveFromThumbnailsFinderQueue(&pListViewInfo,NULL);
	}
	return;
}

/* Draws a thumbnail based on an items icon. */
int CFolderView::GetIconThumbnail(int iInternalIndex)
{
	return GetThumbnailInternal(THUMBNAIL_TYPE_ICON,iInternalIndex,
		NULL);
}

/* Draws an items extracted thumbnail. */
int CFolderView::GetExtractedThumbnail(HBITMAP hThumbnailBitmap)
{
	return GetThumbnailInternal(THUMBNAIL_TYPE_EXTRACTED,0,
		hThumbnailBitmap);
}

int CFolderView::GetThumbnailInternal(int iType,
int iInternalIndex,HBITMAP hThumbnailBitmap)
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

void CFolderView::DrawIconThumbnailInternal(HDC hdcBacking,int iInternalIndex)
{
	LPITEMIDLIST pidlFull = NULL;
	HICON hIcon;
	SHFILEINFO shfi;
	int iIconWidth;
	int iIconHeight;

	pidlFull = ILCombine(m_pidlDirectory,m_pExtraItemInfo[iInternalIndex].pridl);
	SHGetFileInfo((LPCTSTR)pidlFull,0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_SYSICONINDEX);

	hIcon = ImageList_GetIcon(m_hListViewImageList,
		shfi.iIcon,ILD_NORMAL);

	ImageList_GetIconSize(m_hListViewImageList,&iIconWidth,&iIconHeight);

	DrawIconEx(hdcBacking,(THUMBNAIL_ITEM_WIDTH - iIconWidth) / 2,
		(THUMBNAIL_ITEM_HEIGHT - iIconHeight) / 2,hIcon,0,0,
		NULL,NULL,DI_NORMAL);
	DestroyIcon(hIcon);
}

void CFolderView::DrawThumbnailInternal(HDC hdcBacking,HBITMAP hThumbnailBitmap)
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