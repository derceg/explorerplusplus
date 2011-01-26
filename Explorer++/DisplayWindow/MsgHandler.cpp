/******************************************************************
 *
 * Project: DisplayWindow
 * File: MsgHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles GUI messages for the display window.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "DisplayWindow.h"
#include "DisplayWindowInternal.h"
#include "../Helper/Helper.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"

using namespace Gdiplus;

/* These give the position and size of the
main 'folder' icon. */
#define MAIN_ICON_LEFT			20
#define MAIN_ICON_TOP			20
#define MAIN_ICON_WIDTH			48
#define MAIN_ICON_HEIGHT		48

/* Defines the position of thumbnails
for image files. The y-coordinate of
the image origin, but the x-coordinate
can float. */
#define THUMB_IMAGE_TOP			10

/* Determines how much spacing there is
at the top and bottom of the thumbnail. */
#define THUMB_HEIGHT_DELTA		20

list<ThumbnailEntry_t>	g_ThumbnailEntries;

void CDisplayWindow::DrawGradientFill(HDC hdc,RECT *rc,RECT *UpdateRect)
{
	GraphicsPath	Path;
	Point			Center(0,0);
	INT				count = 1;

	if(m_hBitmapBackground)
		DeleteObject(m_hBitmapBackground);

	/* Create the (temporary) off-screen buffer used for drawing. */
	m_hBitmapBackground	= CreateCompatibleBitmap(hdc,rc->right - rc->left,rc->bottom - rc->top);
	SelectObject(m_hdcBackground,m_hBitmapBackground);

	Graphics graphics(m_hdcBackground);

	/* This rectangle encloses the entire client area of the window. */
	Rect DisplayRect(0,0,rc->right - rc->left,rc->bottom - rc->top);

	/* Add the client area to the path objects drawing section,
	and set the centre colour that will be used. */
	Path.AddRectangle(DisplayRect);
	PathGradientBrush pgb(&Path);
	pgb.SetCenterPoint(Center);

	pgb.SetCenterColor(m_CentreColor);
	
	pgb.SetSurroundColors(&m_SurroundColor,&count);
	graphics.FillRectangle(&pgb,DisplayRect);

	/* This draws a seperator line across the top edge of the window,
	so that it is visually seperated from other windows. */
	Pen NewPen(BORDER_COLOUR,1);
	graphics.DrawLine(&NewPen,0,0,rc->right,0);
}

void CDisplayWindow::PatchBackground(HDC hdc,RECT *rc,RECT *UpdateRect)
{
	HDC		hdcMem;
	HBITMAP	hBitmap;

	/* Create the (temporary) off-screen buffer used for drawing. */
	hdcMem	= CreateCompatibleDC(hdc);
	hBitmap	= CreateCompatibleBitmap(hdc,rc->right-rc->left,rc->bottom-rc->top);
	SelectObject(hdcMem,hBitmap);

	/* Draw the stored background on top of the patched area. */
	BitBlt(hdcMem,UpdateRect->left,UpdateRect->top,rc->right,rc->bottom,m_hdcBackground,
	UpdateRect->left,UpdateRect->top,SRCCOPY);

	PaintText(hdcMem,m_LeftIndent,m_LineSpacing);
	DrawIconEx(hdcMem,MAIN_ICON_LEFT,MAIN_ICON_TOP,m_hMainIcon,
		MAIN_ICON_WIDTH,MAIN_ICON_HEIGHT,NULL,NULL,DI_NORMAL);

	if(m_bShowThumbnail)
		DrawThumbnail(hdcMem);

	BitBlt(hdc,UpdateRect->left,UpdateRect->top,rc->right,rc->bottom,hdcMem,
	UpdateRect->left,UpdateRect->top,SRCCOPY);

	DeleteDC(hdcMem);
	DeleteObject(hBitmap);
}

void CDisplayWindow::DrawGradientFillExternal(HDC hdc,RECT *rc,RECT *UpdateRect)
{
	PatchBackground(hdc,rc,UpdateRect);
}

void CDisplayWindow::DrawThumbnail(HDC hdcMem)
{
	HBITMAP hBitmapOld;
	HDC hdcSrc;
	RECT rc;

	if(!m_bThumbnailExtracted)
	{
		ExtractThumbnailImage();
	}
	else
	{
		if(!m_bThumbnailExtractionFailed)
		{
			GetClientRect(m_hDisplayWindow,&rc);

			hdcSrc = CreateCompatibleDC(hdcMem);
			hBitmapOld = (HBITMAP)SelectObject(hdcSrc,m_hbmThumbnail);

			BitBlt(hdcMem,m_xColumnFinal,THUMB_IMAGE_TOP,
				GetRectWidth(&rc) - m_xColumnFinal,GetRectHeight(&rc) - THUMB_HEIGHT_DELTA,
				hdcSrc,0,0,SRCCOPY);

			SelectObject(hdcSrc,hBitmapOld);
			DeleteDC(hdcSrc);
		}
	}
}

DWORD WINAPI Thread_ExtractThumbnailImage(LPVOID lpParameter)
{
	CDisplayWindow *pdw = NULL;

	pdw = (CDisplayWindow *)((ThumbnailEntry_t *)lpParameter)->pdw;

	CoInitializeEx(0,COINIT_APARTMENTTHREADED);

	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);

	pdw->ExtractThumbnailImageInternal((ThumbnailEntry_t *)lpParameter);

	/* TODO: Erase the item. */
	//g_ThumbnailEntries.erase();

	CoUninitialize();

	return 0;
}

void CDisplayWindow::ExtractThumbnailImage(void)
{
	ThumbnailEntry_t te;

	te.pdw			= (void *)this;
	te.bCancelled	= FALSE;
	g_ThumbnailEntries.push_back(te);

	CreateThread(NULL,0,Thread_ExtractThumbnailImage,
		(LPVOID)&g_ThumbnailEntries.back(),0,NULL);
}

void CDisplayWindow::ExtractThumbnailImageInternal(ThumbnailEntry_t *pte)
{
	IExtractImage *pExtractImage = NULL;
	IShellFolder *pShellDesktop = NULL;
	IShellFolder *pShellFolder = NULL;
	LPITEMIDLIST pidlParent = NULL;
	LPITEMIDLIST pidlFull = NULL;
	LPITEMIDLIST pridl = NULL;
	HBITMAP hBitmap;
	BITMAP bm;
	RECT rc;
	SIZE size;
	TCHAR szImage[MAX_PATH];
	DWORD dwPriority;
	DWORD dwFlags;
	HRESULT hr;

	m_bThumbnailExtracted = TRUE;
	m_bThumbnailExtractionFailed = TRUE;

	hr = GetIdlFromParsingName(m_ImageFile,&pidlFull);

	if(SUCCEEDED(hr))
	{
		pidlParent = ILClone(pidlFull);
		ILRemoveLastID(pidlParent);

		pridl = ILClone(ILFindLastID(pidlFull));

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
					GetClientRect(m_hDisplayWindow,&rc);

					/* First, query the thumbnail so that its actual aspect
					ratio can be calculated. */
					dwFlags = IEIFLAG_OFFLINE|IEIFLAG_QUALITY|IEIFLAG_ORIGSIZE;
					size.cx = GetRectHeight(&rc) - THUMB_HEIGHT_DELTA;
					size.cy = GetRectHeight(&rc) - THUMB_HEIGHT_DELTA;

					hr = pExtractImage->GetLocation(szImage,MAX_PATH,
						&dwPriority,&size,32,&dwFlags);

					if(SUCCEEDED(hr))
					{
						hr = pExtractImage->Extract(&hBitmap);

						if(SUCCEEDED(hr))
						{
							/* Get bitmap information (including height and width). */
							GetObject(hBitmap,sizeof(BITMAP),&bm);

							/* Delete the original bitmap. */
							DeleteObject(hBitmap);

							/* ...now query the thumbnail again, this time adjusting
							the width of the suggested area based on the actual aspect
							ratio. */
							dwFlags = IEIFLAG_OFFLINE|IEIFLAG_QUALITY|IEIFLAG_ASPECT|IEIFLAG_ORIGSIZE;
							size.cy = GetRectHeight(&rc) - THUMB_HEIGHT_DELTA;
							size.cx = (LONG)((double)size.cy * ((double)bm.bmWidth / (double)bm.bmHeight));
							m_iImageWidth = size.cx;
							m_iImageHeight = size.cy;
							pExtractImage->GetLocation(szImage,MAX_PATH,
								&dwPriority,&size,32,&dwFlags);
							hr = pExtractImage->Extract(&m_hbmThumbnail);

							if(SUCCEEDED(hr))
							{
								/* Check first if we've been cancelled. This might happen,
								for example, if another file is selected while the current
								thumbnail is been found. */
								EnterCriticalSection(&m_csDWThumbnails);

								if(!pte->bCancelled)
								{
									m_bThumbnailExtractionFailed = FALSE;
									InvalidateRect(m_hDisplayWindow,NULL,FALSE);
								}

								LeaveCriticalSection(&m_csDWThumbnails);
							}
							else
							{
								m_bThumbnailExtractionFailed = TRUE;
								m_hbmThumbnail = NULL;
							}
						}
					}

					pExtractImage->Release();
				}

				pShellFolder->Release();
			}

			pShellDesktop->Release();
		}

		CoTaskMemFree(pidlFull);
		CoTaskMemFree(pidlParent);
		CoTaskMemFree(pridl);
	}
}

void CDisplayWindow::PaintText(HDC hdc,unsigned int x,unsigned int y)
{
	RECT rcClient;
	RECT rcText;
	int xCurrent;
	int iTextBottom;
	int iLine = 0;
	int iCurrentColumnWidth = 0;
	unsigned int i = 0;

	/* Needed to get character widths properly. */
	ApplyDefaultFont(hdc);

	GetClientRect(m_hDisplayWindow,&rcClient);
	xCurrent = x;

	/* TODO: Fix. */
	for(i = 0;i < m_LineList.size();i++)
	{
		SIZE StringSize;

		GetTextExtentPoint32(hdc,m_LineList.at(i).szText,
			lstrlen(m_LineList.at(i).szText),&StringSize);

		iTextBottom = (iLine * StringSize.cy) + m_LineSpacing + StringSize.cy;

		if(abs(iTextBottom - rcClient.bottom) < TEXT_VERTICAL_THRESHOLD)
		{
			xCurrent += iCurrentColumnWidth + TEXT_COLUMN_SPACING;
			iCurrentColumnWidth = 0;
			iLine = 0;
		}

		iCurrentColumnWidth = max(iCurrentColumnWidth,StringSize.cx);

		rcText.left		= xCurrent;
		rcText.top		= (iLine * StringSize.cy) + m_LineSpacing;
		rcText.right	= rcText.left + StringSize.cx;
		rcText.bottom	= rcText.top + StringSize.cy;
		TransparentTextOut(hdc,m_LineList.at(i).szText,&rcText);

		iLine++;
	}

	/* Needed for thumbnail image. */
	xCurrent += iCurrentColumnWidth + TEXT_COLUMN_SPACING;
	m_xColumnFinal = xCurrent;
}

void CDisplayWindow::TransparentTextOut(HDC hdc,TCHAR *Text,RECT *prcText)
{
	ApplyDefaultFont(hdc);

	DrawText(hdc,Text,lstrlen(Text),prcText,DT_LEFT|DT_NOPREFIX);
}

void CDisplayWindow::ApplyDefaultFont(HDC hdc)
{
	SelectObject(hdc,m_hDisplayFont);

	SetBkMode(hdc,TRANSPARENT);
	SetTextColor(hdc,m_TextColor);
}

LONG CDisplayWindow::OnMouseMove(LPARAM lParam)
{
	POINT			CursorPos;
	static POINT	PrevCursorPos;
	RECT			rc;
	RECT			rc2;

	CursorPos.x = GET_X_LPARAM(lParam);
	CursorPos.y = GET_Y_LPARAM(lParam);

	GetClientRect(m_hDisplayWindow,&rc);

	GetClientRect(GetParent(m_hDisplayWindow),&rc2);

	if(m_bSizing)
	{
		if((PrevCursorPos.x == CursorPos.x)
		&& (PrevCursorPos.y == CursorPos.y))
			return 0;

		PrevCursorPos.x = CursorPos.x;
		PrevCursorPos.y = CursorPos.y;

		/* Notify the main window, so that it can redraw/reposition
		its other windows. */
		SendMessage(GetParent(m_hDisplayWindow),
			WM_USER_DISPLAYWINDOWRESIZED,(WPARAM)(rc.bottom - CursorPos.y),0);
	}

	if(CursorPos.y <= (rc.top + 5))
	{
		SetCursor(LoadCursor(NULL,IDC_SIZENS));
	}

	/* If there is a thumbnail preview
	for an image been shown, and the
	mouse is over the thumbnail, set the
	mouse pointer to a hand. */
	if(m_bShowThumbnail)
	{
		RECT rcThumbnail;

		SetRect(&rcThumbnail,m_xColumnFinal,
		THUMB_IMAGE_TOP,m_xColumnFinal + m_iImageWidth,
		THUMB_IMAGE_TOP + m_iImageHeight);

		if(PtInRect(&rcThumbnail,CursorPos))
		{
			SetCursor(LoadCursor(NULL,IDC_HAND));
		}
	}

	return 1;
}

void CDisplayWindow::OnLButtonDown(LPARAM lParam)
{
	POINT	CursorPos;
	RECT	rc;

	CursorPos.x = GET_X_LPARAM(lParam);
	CursorPos.y = GET_Y_LPARAM(lParam);

	GetClientRect(m_hDisplayWindow,&rc);

	if(CursorPos.y <= (rc.top + 5))
	{
		SetCursor(LoadCursor(NULL,IDC_SIZENS));
		m_bSizing = TRUE;
		SetFocus(m_hDisplayWindow);
		SetCapture(m_hDisplayWindow);
	}

	/* If an image thumbnial was clicked, open
	the image and set the mouse pointer back to
	the regular pointer. */
	if(m_bShowThumbnail)
	{
		RECT rcThumbnail;

		SetRect(&rcThumbnail,m_xColumnFinal,
		THUMB_IMAGE_TOP,m_xColumnFinal + m_iImageWidth,
		THUMB_IMAGE_TOP + m_iImageHeight);

		if(PtInRect(&rcThumbnail,CursorPos))
		{
			SetCursor(LoadCursor(NULL,IDC_HAND));
			ShellExecute(m_hDisplayWindow,_T("open"),
			m_ImageFile,NULL,NULL,SW_SHOW);
		}
	}
}

void CDisplayWindow::OnRButtonUp(WPARAM wParam,LPARAM lParam)
{
	POINT pt;
	RECT rc;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);

	rc.left		= MAIN_ICON_LEFT;
	rc.top		= MAIN_ICON_TOP;
	rc.right	= MAIN_ICON_LEFT + MAIN_ICON_WIDTH;
	rc.bottom	= MAIN_ICON_TOP + MAIN_ICON_HEIGHT;

	if(PtInRect(&rc,pt))
	{
		SendMessage(GetParent(m_hDisplayWindow),WM_NDW_ICONRCLICK,wParam,lParam);
	}
	else
	{
		SendMessage(GetParent(m_hDisplayWindow),WM_NDW_RCLICK,wParam,lParam);
	}
}

void CDisplayWindow::CancelThumbnailExtraction(void)
{
	list<ThumbnailEntry_t>::iterator itr;

	EnterCriticalSection(&m_csDWThumbnails);

	for(itr = g_ThumbnailEntries.begin();itr != g_ThumbnailEntries.end();itr++)
	{
		itr->bCancelled = TRUE;
	}

	LeaveCriticalSection(&m_csDWThumbnails);
}

void CDisplayWindow::OnSetThumbnailFile(WPARAM wParam,LPARAM lParam)
{
	m_bShowThumbnail = (BOOL)lParam;

	if(m_bShowThumbnail)
	{
		CancelThumbnailExtraction();

		if(m_hbmThumbnail)
			DeleteObject(m_hbmThumbnail);

		m_iImageWidth	= 0;
		m_iImageHeight	= 0;
		m_bThumbnailExtracted = FALSE;
		m_bThumbnailExtractionFailed = FALSE;
		StringCchCopy(m_ImageFile,SIZEOF_ARRAY(m_ImageFile),
		(TCHAR *)wParam);
	}
}

void CDisplayWindow::OnSize(WPARAM wParam,LPARAM lParam)
{
	HDC		hdc;
	RECT	rc;

	hdc = GetDC(m_hDisplayWindow);

	SetRect(&rc,0,0,LOWORD(lParam),HIWORD(lParam));
	DrawGradientFill(hdc,&rc,NULL);

	ReleaseDC(m_hDisplayWindow,hdc);

	RedrawWindow(m_hDisplayWindow,NULL,NULL,RDW_INVALIDATE);
}

void CDisplayWindow::OnSetFont(HFONT hFont)
{
	m_hDisplayFont = hFont;

	RedrawWindow(m_hDisplayWindow,NULL,NULL,RDW_INVALIDATE);
}

void CDisplayWindow::OnSetTextColor(COLORREF hColor)
{
	m_TextColor = hColor;

	RedrawWindow(m_hDisplayWindow,NULL,NULL,RDW_INVALIDATE);
}