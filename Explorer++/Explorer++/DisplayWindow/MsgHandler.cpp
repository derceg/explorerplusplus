// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DisplayWindow.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"

/* Defines how close the text can get to the bottom
of the display window before it is moved into the
next column. */
#define TEXT_VERTICAL_THRESHOLD 10

/* Defines the horizontal spacing between text columns. */
#define TEXT_COLUMN_SPACING 50

/* These give the position and size of the
main 'folder' icon. */
#define MAIN_ICON_LEFT 20
#define MAIN_ICON_TOP 20
#define MAIN_ICON_WIDTH 48
#define MAIN_ICON_HEIGHT 48

/* Defines the position of thumbnails
for image files. The y-coordinate of
the image origin, but the x-coordinate
can float. */
#define THUMB_IMAGE_TOP 10

/* Determines how much spacing there is
at the top and bottom of the thumbnail. */
#define THUMB_HEIGHT_DELTA 20

std::list<ThumbnailEntry_t> g_ThumbnailEntries;

void DisplayWindow::DrawGradientFill(HDC hdc, RECT *rc)
{
	if (m_hBitmapBackground)
	{
		DeleteObject(m_hBitmapBackground);
	}

	/* Create the (temporary) off-screen buffer used for drawing. */
	m_hBitmapBackground = CreateCompatibleBitmap(hdc, rc->right - rc->left, rc->bottom - rc->top);
	HGDIOBJ originalBackgroundObject = SelectObject(m_hdcBackground, m_hBitmapBackground);

	Gdiplus::Graphics graphics(m_hdcBackground);

	Gdiplus::Rect displayRect(0, 0, rc->right - rc->left, rc->bottom - rc->top);

	Gdiplus::GraphicsPath path;
	path.AddRectangle(displayRect);
	Gdiplus::PathGradientBrush pgb(&path);
	pgb.SetCenterPoint(Gdiplus::Point(0, 0));

	pgb.SetCenterColor(m_CentreColor);

	INT count = 1;
	pgb.SetSurroundColors(&m_SurroundColor, &count);
	graphics.FillRectangle(&pgb, displayRect);

	/* This draws a separator line across the top edge of the window,
	so that it is visually separated from other windows. */
	Gdiplus::Pen newPen(BORDER_COLOUR, 1);

	if (m_bVertical)
	{
		graphics.DrawLine(&newPen, 0, 0, 0, rc->bottom);
	}
	else
	{
		graphics.DrawLine(&newPen, 0, 0, rc->right, 0);
	}

	SelectObject(m_hdcBackground, originalBackgroundObject);
}

void DisplayWindow::PatchBackground(HDC hdc, RECT *rc, RECT *updateRect)
{
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rc->right - rc->left, rc->bottom - rc->top);
	HGDIOBJ hOriginalObject = SelectObject(hdcMem, hBitmap);

	/* Draw the stored background on top of the patched area. */
	BitBlt(hdcMem, updateRect->left, updateRect->top, rc->right, rc->bottom, m_hdcBackground,
		updateRect->left, updateRect->top, SRCCOPY);

	PaintText(hdcMem, m_LeftIndent);
	DrawIconEx(hdcMem, MAIN_ICON_LEFT, MAIN_ICON_TOP, m_hMainIcon, MAIN_ICON_WIDTH,
		MAIN_ICON_HEIGHT, 0, nullptr, DI_NORMAL);

	if (m_bShowThumbnail)
	{
		DrawThumbnail(hdcMem);
	}

	BitBlt(hdc, updateRect->left, updateRect->top, rc->right, rc->bottom, hdcMem, updateRect->left,
		updateRect->top, SRCCOPY);

	SelectObject(hdcMem, hOriginalObject);
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
}

void DisplayWindow::DrawThumbnail(HDC hdcMem)
{
	if (!m_bThumbnailExtracted)
	{
		ExtractThumbnailImage();
	}
	else
	{
		if (!m_bThumbnailExtractionFailed)
		{
			RECT rc;
			GetClientRect(m_hDisplayWindow, &rc);

			HDC hdcSrc = CreateCompatibleDC(hdcMem);
			auto hBitmapOld = (HBITMAP) SelectObject(hdcSrc, m_hbmThumbnail);

			BitBlt(hdcMem, m_xColumnFinal, THUMB_IMAGE_TOP, GetRectWidth(&rc) - m_xColumnFinal,
				GetRectHeight(&rc) - THUMB_HEIGHT_DELTA, hdcSrc, 0, 0, SRCCOPY);

			SelectObject(hdcSrc, hBitmapOld);
			DeleteDC(hdcSrc);
		}
	}
}

DWORD WINAPI Thread_ExtractThumbnailImage(LPVOID lpParameter)
{
	DisplayWindow *pdw = nullptr;

	pdw = (DisplayWindow *) ((ThumbnailEntry_t *) lpParameter)->pdw;

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	pdw->ExtractThumbnailImageInternal((ThumbnailEntry_t *) lpParameter);

	/* TODO: Erase the item. */
	// g_ThumbnailEntries.erase();

	CoUninitialize();

	return 0;
}

void DisplayWindow::ExtractThumbnailImage()
{
	ThumbnailEntry_t te;

	te.pdw = (void *) this;
	te.bCancelled = FALSE;
	g_ThumbnailEntries.push_back(te);

	HANDLE hThread = CreateThread(nullptr, 0, Thread_ExtractThumbnailImage,
		(LPVOID) &g_ThumbnailEntries.back(), 0, nullptr);
	CloseHandle(hThread);
}

void DisplayWindow::ExtractThumbnailImageInternal(ThumbnailEntry_t *pte)
{
	IExtractImage *pExtractImage = nullptr;
	IShellFolder *pShellFolder = nullptr;
	PIDLIST_ABSOLUTE pidlParent = nullptr;
	PIDLIST_ABSOLUTE pidlFull = nullptr;
	PITEMID_CHILD pridl = nullptr;
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

	hr = SHParseDisplayName(m_ImageFile, nullptr, &pidlFull, 0, nullptr);

	if (SUCCEEDED(hr))
	{
		pidlParent = ILCloneFull(pidlFull);
		ILRemoveLastID(pidlParent);

		pridl = ILCloneChild(ILFindLastID(pidlFull));

		hr = BindToIdl(pidlParent, IID_PPV_ARGS(&pShellFolder));

		if (SUCCEEDED(hr))
		{
			hr = GetUIObjectOf(pShellFolder, nullptr, 1, &pridl, IID_PPV_ARGS(&pExtractImage));

			if (SUCCEEDED(hr))
			{
				GetClientRect(m_hDisplayWindow, &rc);

				/* First, query the thumbnail so that its actual aspect
				ratio can be calculated. */
				dwFlags = IEIFLAG_OFFLINE | IEIFLAG_QUALITY | IEIFLAG_ORIGSIZE;
				size.cx = GetRectHeight(&rc) - THUMB_HEIGHT_DELTA;
				size.cy = GetRectHeight(&rc) - THUMB_HEIGHT_DELTA;

				hr = pExtractImage->GetLocation(szImage, SIZEOF_ARRAY(szImage), &dwPriority, &size,
					32, &dwFlags);

				if (SUCCEEDED(hr))
				{
					hr = pExtractImage->Extract(&hBitmap);

					if (SUCCEEDED(hr))
					{
						/* Get bitmap information (including height and width). */
						GetObject(hBitmap, sizeof(BITMAP), &bm);

						/* Delete the original bitmap. */
						DeleteObject(hBitmap);

						/* ...now query the thumbnail again, this time adjusting
						the width of the suggested area based on the actual aspect
						ratio. */
						dwFlags =
							IEIFLAG_OFFLINE | IEIFLAG_QUALITY | IEIFLAG_ASPECT | IEIFLAG_ORIGSIZE;
						size.cy = GetRectHeight(&rc) - THUMB_HEIGHT_DELTA;
						size.cx = (LONG) ((double) size.cy
							* ((double) bm.bmWidth / (double) bm.bmHeight));
						m_iImageWidth = size.cx;
						m_iImageHeight = size.cy;
						pExtractImage->GetLocation(szImage, SIZEOF_ARRAY(szImage), &dwPriority,
							&size, 32, &dwFlags);
						hr = pExtractImage->Extract(&m_hbmThumbnail);

						if (SUCCEEDED(hr))
						{
							/* Check first if we've been cancelled. This might happen,
							for example, if another file is selected while the current
							thumbnail is been found. */
							EnterCriticalSection(&m_csDWThumbnails);

							if (!pte->bCancelled)
							{
								m_bThumbnailExtractionFailed = FALSE;
								InvalidateRect(m_hDisplayWindow, nullptr, FALSE);
							}

							LeaveCriticalSection(&m_csDWThumbnails);
						}
						else
						{
							m_bThumbnailExtractionFailed = TRUE;
							m_hbmThumbnail = nullptr;
						}
					}
				}

				pExtractImage->Release();
			}

			pShellFolder->Release();
		}

		CoTaskMemFree(pidlFull);
		CoTaskMemFree(pidlParent);
		CoTaskMemFree(pridl);
	}
}

void DisplayWindow::PaintText(HDC hdc, unsigned int x)
{
	RECT rcClient;
	RECT rcText;
	int xCurrent;
	int iTextBottom;
	int iLine = 0;
	int iCurrentColumnWidth = 0;
	unsigned int i = 0;

	/* Needed to get character widths properly. */
	HGDIOBJ hOriginalObject = SelectObject(hdc, m_hDisplayFont);

	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, m_TextColor);

	GetClientRect(m_hDisplayWindow, &rcClient);
	xCurrent = x;

	/* TODO: Fix. */
	for (i = 0; i < m_LineList.size(); i++)
	{
		SIZE stringSize;

		GetTextExtentPoint32(hdc, m_LineList.at(i).szText, lstrlen(m_LineList.at(i).szText),
			&stringSize);

		iTextBottom = (iLine * stringSize.cy) + m_LineSpacing + stringSize.cy;

		if (abs(iTextBottom - rcClient.bottom) < TEXT_VERTICAL_THRESHOLD)
		{
			xCurrent += iCurrentColumnWidth + TEXT_COLUMN_SPACING;
			iCurrentColumnWidth = 0;
			iLine = 0;
		}

		iCurrentColumnWidth = max(iCurrentColumnWidth, stringSize.cx);

		rcText.left = xCurrent;
		rcText.top = (iLine * stringSize.cy) + m_LineSpacing;
		rcText.right = rcText.left + stringSize.cx;
		rcText.bottom = rcText.top + stringSize.cy;
		TransparentTextOut(hdc, m_LineList.at(i).szText, &rcText);

		iLine++;
	}

	/* Needed for thumbnail image. */
	xCurrent += iCurrentColumnWidth + TEXT_COLUMN_SPACING;
	m_xColumnFinal = xCurrent;

	SelectObject(hdc, hOriginalObject);
}

void DisplayWindow::TransparentTextOut(HDC hdc, TCHAR *text, RECT *prcText)
{
	DrawText(hdc, text, lstrlen(text), prcText, DT_LEFT | DT_NOPREFIX);
}

LONG DisplayWindow::OnMouseMove(LPARAM lParam)
{
	POINT cursorPos;
	static POINT prevCursorPos;
	RECT rc;
	RECT rc2;

	cursorPos.x = GET_X_LPARAM(lParam);
	cursorPos.y = GET_Y_LPARAM(lParam);

	GetClientRect(m_hDisplayWindow, &rc);

	GetClientRect(GetParent(m_hDisplayWindow), &rc2);

	if (m_bSizing)
	{
		if ((prevCursorPos.x == cursorPos.x) && (prevCursorPos.y == cursorPos.y))
		{
			return 0;
		}

		prevCursorPos.x = cursorPos.x;
		prevCursorPos.y = cursorPos.y;

		/* Notify the main window, so that it can redraw/reposition
		its other windows. */
		SendMessage(GetParent(m_hDisplayWindow), WM_USER_DISPLAYWINDOWRESIZED,
			MAKEWPARAM(max(rc.right - cursorPos.x, 0), max(rc.bottom - cursorPos.y, 0)), 0);
	}

	if (m_bVertical && cursorPos.x <= (rc.left + 5) || !m_bVertical && cursorPos.y <= (rc.top + 5))
	{
		SetCursor(LoadCursor(NULL, m_bVertical ? IDC_SIZEWE : IDC_SIZENS));
	}

	/* If there is a thumbnail preview
	for an image been shown, and the
	mouse is over the thumbnail, set the
	mouse pointer to a hand. */
	if (m_bShowThumbnail)
	{
		RECT rcThumbnail;

		SetRect(&rcThumbnail, m_xColumnFinal, THUMB_IMAGE_TOP, m_xColumnFinal + m_iImageWidth,
			THUMB_IMAGE_TOP + m_iImageHeight);

		if (PtInRect(&rcThumbnail, cursorPos))
		{
			SetCursor(LoadCursor(nullptr, IDC_HAND));
		}
	}

	return 1;
}

void DisplayWindow::OnLButtonDown(LPARAM lParam)
{
	POINT cursorPos;
	RECT rc;

	cursorPos.x = GET_X_LPARAM(lParam);
	cursorPos.y = GET_Y_LPARAM(lParam);

	GetClientRect(m_hDisplayWindow, &rc);

	if (m_bVertical && cursorPos.x <= (rc.left + 5) || !m_bVertical && cursorPos.y <= (rc.top + 5))
	{
		SetCursor(LoadCursor(NULL, m_bVertical ? IDC_SIZEWE : IDC_SIZENS));
		m_bSizing = TRUE;
		SetFocus(m_hDisplayWindow);
		SetCapture(m_hDisplayWindow);
	}

	/* If an image thumbnail was clicked, open
	the image and set the mouse pointer back to
	the regular pointer. */
	if (m_bShowThumbnail)
	{
		RECT rcThumbnail;

		SetRect(&rcThumbnail, m_xColumnFinal, THUMB_IMAGE_TOP, m_xColumnFinal + m_iImageWidth,
			THUMB_IMAGE_TOP + m_iImageHeight);

		if (PtInRect(&rcThumbnail, cursorPos))
		{
			/* TODO: Parent should be notified. */
			SetCursor(LoadCursor(nullptr, IDC_HAND));
			ShellExecute(m_hDisplayWindow, _T("open"), m_ImageFile, nullptr, nullptr,
				SW_SHOWNORMAL);
		}
	}
}

void DisplayWindow::OnRButtonUp(WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	RECT rc;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);

	rc.left = MAIN_ICON_LEFT;
	rc.top = MAIN_ICON_TOP;
	rc.right = MAIN_ICON_LEFT + MAIN_ICON_WIDTH;
	rc.bottom = MAIN_ICON_TOP + MAIN_ICON_HEIGHT;

	if (PtInRect(&rc, pt))
	{
		SendMessage(GetParent(m_hDisplayWindow), WM_NDW_ICONRCLICK, wParam, lParam);
	}
	else
	{
		SendMessage(GetParent(m_hDisplayWindow), WM_NDW_RCLICK, wParam, lParam);
	}
}

void DisplayWindow::CancelThumbnailExtraction()
{
	std::list<ThumbnailEntry_t>::iterator itr;

	EnterCriticalSection(&m_csDWThumbnails);

	for (itr = g_ThumbnailEntries.begin(); itr != g_ThumbnailEntries.end(); itr++)
	{
		itr->bCancelled = TRUE;
	}

	LeaveCriticalSection(&m_csDWThumbnails);
}

void DisplayWindow::OnSetThumbnailFile(WPARAM wParam, LPARAM lParam)
{
	m_bShowThumbnail = (BOOL) lParam;

	if (m_bShowThumbnail)
	{
		CancelThumbnailExtraction();

		if (m_hbmThumbnail)
		{
			DeleteObject(m_hbmThumbnail);
		}

		m_iImageWidth = 0;
		m_iImageHeight = 0;
		m_bThumbnailExtracted = FALSE;
		m_bThumbnailExtractionFailed = FALSE;
		StringCchCopy(m_ImageFile, SIZEOF_ARRAY(m_ImageFile), (TCHAR *) wParam);
	}
}

void DisplayWindow::OnSize(int width, int height)
{
	HDC hdc;
	RECT rc;

	hdc = GetDC(m_hDisplayWindow);

	SetRect(&rc, 0, 0, width, height);
	DrawGradientFill(hdc, &rc);

	ReleaseDC(m_hDisplayWindow, hdc);

	RedrawWindow(m_hDisplayWindow, nullptr, nullptr, RDW_INVALIDATE);
}

void DisplayWindow::OnSetFont(HFONT hFont)
{
	m_hDisplayFont = hFont;

	RedrawWindow(m_hDisplayWindow, nullptr, nullptr, RDW_INVALIDATE);
}

void DisplayWindow::OnSetTextColor(COLORREF hColor)
{
	m_TextColor = hColor;

	RedrawWindow(m_hDisplayWindow, nullptr, nullptr, RDW_INVALIDATE);
}
