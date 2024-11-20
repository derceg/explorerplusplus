// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <gdiplus.h>
#include <memory>
#include <vector>

#define DWM_BASE (WM_APP + 100)

#define DWM_SETTHUMBNAILFILE (DWM_BASE + 2)
#define DWM_GETSURROUNDCOLOR (DWM_BASE + 3)
#define DWM_SETSURROUNDCOLOR (DWM_BASE + 4)
#define DWM_GETCENTRECOLOR (DWM_BASE + 7)
#define DWM_SETCENTRECOLOR (DWM_BASE + 8)
#define DWM_DRAWGRADIENTFILL (DWM_BASE + 10)
#define DWM_GETFONT (DWM_BASE + 11)
#define DWM_SETFONT (DWM_BASE + 12)
#define DWM_GETTEXTCOLOR (DWM_BASE + 13)
#define DWM_SETTEXTCOLOR (DWM_BASE + 14)
#define DWM_BUFFERTEXT (DWM_BASE + 15)
#define DWM_CLEARTEXTBUFFER (DWM_BASE + 16)
#define DWM_SETLINE (DWM_BASE + 17)

#define DisplayWindow_SetThumbnailFile(hDisplay, FileName, bShowImage)                             \
	SendMessage(hDisplay, DWM_SETTHUMBNAILFILE, (WPARAM) FileName, bShowImage)

#define DisplayWindow_GetSurroundColor(hDisplay) SendMessage(hDisplay, DWM_GETSURROUNDCOLOR, 0, 0)

#define DisplayWindow_SetColors(hDisplay, rgbColor)                                                \
	SendMessage(hDisplay, DWM_SETSURROUNDCOLOR, rgbColor, 0)

#define DisplayWindow_SetFont(hDisplay, hFont) SendMessage(hDisplay, DWM_SETFONT, hFont, 0)

#define DisplayWindow_GetFont(hDisplay, hFont) SendMessage(hDisplay, DWM_GETFONT, hFont, 0)

#define DisplayWindow_GetTextColor(hDisplay)                                                       \
	(COLORREF) SendMessage(hDisplay, DWM_GETTEXTCOLOR, 0, 0)

#define DisplayWindow_SetTextColor(hDisplay, hColor)                                               \
	SendMessage(hDisplay, DWM_SETTEXTCOLOR, hColor, 0)

#define DisplayWindow_BufferText(hDisplay, szText)                                                 \
	SendMessage(hDisplay, DWM_BUFFERTEXT, 0, (LPARAM) szText)

#define DisplayWindow_ClearTextBuffer(hDisplay) SendMessage(hDisplay, DWM_CLEARTEXTBUFFER, 0, 0)

#define DisplayWindow_SetLine(hDisplay, iLine, szText)                                             \
	SendMessage(hDisplay, DWM_SETLINE, iLine, (LPARAM) szText)

#define WM_USER_DISPLAYWINDOWMOVED (WM_APP + 99)
#define WM_USER_DISPLAYWINDOWRESIZED (WM_APP + 100)

#define WM_NDW_ICONRCLICK (WM_APP + 101)
#define WM_NDW_RCLICK (WM_APP + 102)

class WindowSubclassWrapper;

typedef struct
{
	Gdiplus::Color CentreColor;
	Gdiplus::Color SurroundColor;
	COLORREF TextColor;
	HFONT hFont;
	HICON hIcon;
} DWInitialSettings_t;

typedef struct
{
	TCHAR szText[512];
} LineData_t;

typedef struct
{
	void *pdw;
	BOOL bCancelled;
} ThumbnailEntry_t;

class DisplayWindow
{
public:
	static DisplayWindow *Create(HWND parent, DWInitialSettings_t *initialSettings);

	HWND GetHWND() const;

	void ExtractThumbnailImageInternal(ThumbnailEntry_t *pte);

private:
	static inline const Gdiplus::Color BORDER_COLOUR{ 128, 128, 128 };

	static inline const wchar_t CLASS_NAME[] = L"DisplayWindow";
	static inline const wchar_t WINDOW_NAME[] = L"DisplayWindow";

	DisplayWindow(HWND parent, DWInitialSettings_t *initialSettings);
	~DisplayWindow();

	static HWND CreateDisplayWindow(HWND parent);
	static ATOM RegisterDisplayWindowClass();

	LRESULT CALLBACK DisplayWindowProc(HWND displayWindow, UINT msg, WPARAM wParam, LPARAM lParam);

	LONG OnMouseMove(LPARAM lParam);
	void OnLButtonDown(LPARAM lParam);
	void OnRButtonUp(WPARAM wParam, LPARAM lParam);
	void DrawGradientFill(HDC, RECT *);
	void PaintText(HDC, unsigned int);
	void TransparentTextOut(HDC hdc, TCHAR *text, RECT *prcText);
	void DrawThumbnail(HDC hdcMem);
	void OnSetThumbnailFile(WPARAM wParam, LPARAM lParam);
	void OnSetFont(HFONT hFont);
	void OnSetTextColor(COLORREF hColor);

	void PatchBackground(HDC hdc, RECT *rc, RECT *updateRect);

	void OnSize(int width, int height);

	void ExtractThumbnailImage();
	void CancelThumbnailExtraction();

	const HWND m_hwnd;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;

	/* Text drawing attributes. */
	COLORREF m_TextColor;
	unsigned int m_LineSpacing;
	unsigned int m_LeftIndent;

	/* Text buffers (for internal redrawing operations). */
	std::vector<LineData_t> m_LineList;
	TCHAR m_ImageFile[MAX_PATH];
	BOOL m_bSizing;
	Gdiplus::Color m_CentreColor;
	Gdiplus::Color m_SurroundColor;

	int m_iImageWidth;
	int m_iImageHeight;
	BOOL m_bVertical;

	/* Thumbnails. */
	CRITICAL_SECTION m_csDWThumbnails;
	HBITMAP m_hbmThumbnail;
	BOOL m_bShowThumbnail;
	BOOL m_bThumbnailExtracted;
	BOOL m_bThumbnailExtractionFailed;

	int m_xColumnFinal;

	HDC m_hdcBackground;
	HBITMAP m_hBitmapBackground;
	HICON m_hMainIcon;
	HFONT m_hDisplayFont;
};
