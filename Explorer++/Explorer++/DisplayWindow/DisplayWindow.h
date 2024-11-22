// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <gdiplus.h>
#include <memory>
#include <vector>

#define DWM_BASE (WM_APP + 100)

#define DWM_SETTHUMBNAILFILE (DWM_BASE + 2)
#define DWM_BUFFERTEXT (DWM_BASE + 15)
#define DWM_CLEARTEXTBUFFER (DWM_BASE + 16)
#define DWM_SETLINE (DWM_BASE + 17)

#define DisplayWindow_SetThumbnailFile(hDisplay, FileName, bShowImage)                             \
	SendMessage(hDisplay, DWM_SETTHUMBNAILFILE, (WPARAM) FileName, bShowImage)

#define DisplayWindow_BufferText(hDisplay, szText)                                                 \
	SendMessage(hDisplay, DWM_BUFFERTEXT, 0, (LPARAM) szText)

#define DisplayWindow_ClearTextBuffer(hDisplay) SendMessage(hDisplay, DWM_CLEARTEXTBUFFER, 0, 0)

#define DisplayWindow_SetLine(hDisplay, iLine, szText)                                             \
	SendMessage(hDisplay, DWM_SETLINE, iLine, (LPARAM) szText)

#define WM_USER_DISPLAYWINDOWMOVED (WM_APP + 99)
#define WM_USER_DISPLAYWINDOWRESIZED (WM_APP + 100)

#define WM_NDW_ICONRCLICK (WM_APP + 101)
#define WM_NDW_RCLICK (WM_APP + 102)

struct Config;
class WindowSubclassWrapper;

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
	static DisplayWindow *Create(HWND parent, const Config *config);

	HWND GetHWND() const;

	void ExtractThumbnailImageInternal(ThumbnailEntry_t *pte);

private:
	static inline const Gdiplus::Color BORDER_COLOUR{ 128, 128, 128 };

	static inline const wchar_t CLASS_NAME[] = L"DisplayWindow";
	static inline const wchar_t WINDOW_NAME[] = L"DisplayWindow";

	DisplayWindow(HWND parent, const Config *config);
	~DisplayWindow();

	static HWND CreateDisplayWindow(HWND parent);
	static ATOM RegisterDisplayWindowClass();

	LRESULT CALLBACK DisplayWindowProc(HWND displayWindow, UINT msg, WPARAM wParam, LPARAM lParam);

	LONG OnMouseMove(LPARAM lParam);
	void OnLButtonDown(LPARAM lParam);
	void OnRButtonUp(WPARAM wParam, LPARAM lParam);
	void PaintText(HDC, unsigned int);
	void TransparentTextOut(HDC hdc, TCHAR *text, RECT *prcText);
	void DrawThumbnail(HDC hdcMem);
	void OnSetThumbnailFile(WPARAM wParam, LPARAM lParam);

	void Draw(HDC hdc, RECT *rc, RECT *updateRect);
	void DrawBackground(HDC hdcMem, RECT *rc);

	void ExtractThumbnailImage();
	void CancelThumbnailExtraction();

	void OnDisplayConfigChanged();
	void OnFontConfigChanged();

	const HWND m_hwnd;
	std::vector<std::unique_ptr<WindowSubclassWrapper>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
	const Config *const m_config;

	wil::unique_hicon m_mainIcon;
	wil::unique_hfont m_font;

	/* Text drawing attributes. */
	unsigned int m_LineSpacing;
	unsigned int m_LeftIndent;

	/* Text buffers (for internal redrawing operations). */
	std::vector<LineData_t> m_LineList;
	TCHAR m_ImageFile[MAX_PATH];
	BOOL m_bSizing;

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
};
