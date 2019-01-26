/******************************************************************
 *
 * Project: DisplayWindow
 * File: DisplayWindow.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Creates and manages a 'display window'. This window
 * can be used to show various information (e.g. file
 * information) and is drawn with a gradient background.
 *
 * Icon:
 * The icon passed in when this class is created will be
 * drawn at 48x48 pixels. To avoid drawing issues, the
 * icon itself should be 48x48 pixels.
 *
 * Text drawing:
 * Text will be drawn in columns. Text height (and width)
 * is calculated, and once a particular column is full,
 * text will be placed in the next column.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "DisplayWindow.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"


namespace
{
	static const TCHAR CLASS_NAME[] = _T("DisplayWindow");
	static const TCHAR WINDOW_NAME[] = _T("DisplayWindow");
}

ULONG_PTR token;

namespace
{
	BOOL RegisterDisplayWindowClass()
	{
		Gdiplus::GdiplusStartupInput	startupInput;

		WNDCLASS wc;
		wc.style			= 0;
		wc.lpfnWndProc		= DisplayWindowProcStub;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= sizeof(CDisplayWindow *);
		wc.hInstance		= GetModuleHandle(NULL);
		wc.hIcon			= NULL;
		wc.hCursor			= LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground	= NULL;
		wc.lpszMenuName		= NULL;
		wc.lpszClassName	= CLASS_NAME;

		if(!RegisterClass(&wc))
		{
			return FALSE;
		}

		Gdiplus::GdiplusStartup(&token,&startupInput,NULL);

		return TRUE;
	}
}

HWND CreateDisplayWindow(HWND Parent,DWInitialSettings_t *pSettings)
{
	RegisterDisplayWindowClass();

	HWND hDisplayWindow = CreateWindow(WINDOW_NAME,EMPTY_STRING,
		WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS,0,0,0,0,
		Parent,NULL,GetModuleHandle(NULL),reinterpret_cast<LPVOID>(pSettings));

	return hDisplayWindow;
}

CDisplayWindow::CDisplayWindow(HWND hDisplayWindow,DWInitialSettings_t *pInitialSettings) :
	m_hDisplayWindow(hDisplayWindow),
	m_hMainIcon(pInitialSettings->hIcon),
	m_CentreColor(pInitialSettings->CentreColor),
	m_SurroundColor(pInitialSettings->SurroundColor),
	m_TextColor(pInitialSettings->TextColor),
	m_hDisplayFont(pInitialSettings->hFont)
{
	g_ObjectCount++;

	m_LineSpacing	= 20;
	m_LeftIndent	= 80;

	m_bSizing = FALSE;
	m_hbmThumbnail = NULL;
	m_bShowThumbnail = FALSE;
	m_bThumbnailExtracted = FALSE;
	m_bThumbnailExtractionFailed = FALSE;
	m_hBitmapBackground = NULL;

	InitializeCriticalSection(&m_csDWThumbnails);
}

CDisplayWindow::~CDisplayWindow()
{
	DeleteCriticalSection(&m_csDWThumbnails);

	DeleteDC(m_hdcBackground);
	DeleteObject(m_hBitmapBackground);

	DestroyIcon(m_hMainIcon);

	g_ObjectCount--;

	if(g_ObjectCount == 0)
	{
		Gdiplus::GdiplusShutdown(token);
	}
}

LRESULT CALLBACK DisplayWindowProcStub(HWND DisplayWindow,UINT msg,
WPARAM wParam,LPARAM lParam)
{
	CDisplayWindow *pdw = reinterpret_cast<CDisplayWindow *>(GetWindowLongPtr(DisplayWindow,GWLP_USERDATA));

	switch(msg)
	{
		case WM_CREATE:
			{
				DWInitialSettings_t *pSettings = reinterpret_cast<DWInitialSettings_t *>(
					reinterpret_cast<CREATESTRUCT *>(lParam)->lpCreateParams);

				pdw = new CDisplayWindow(DisplayWindow,pSettings);
				SetWindowLongPtr(DisplayWindow,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pdw));
			}
			break;

		case WM_NCDESTROY:
			delete pdw;
			return 0;
			break;
	}

	return pdw->DisplayWindowProc(DisplayWindow,msg,wParam,lParam);
}

LRESULT CALLBACK CDisplayWindow::DisplayWindowProc(HWND DisplayWindow,UINT msg,
WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_CREATE:
			m_hdcBackground	= CreateCompatibleDC(GetDC(DisplayWindow));
			break;

		case WM_LBUTTONUP:
			m_bSizing = FALSE;
			ReleaseCapture();
			break;

		case WM_MOUSEMOVE:
			return OnMouseMove(lParam);
			break;

		case WM_LBUTTONDOWN:
			OnLButtonDown(lParam);
			break;

		case WM_RBUTTONUP:
			OnRButtonUp(wParam,lParam);
			break;

		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc;
				RECT UpdateRect;
				RECT rc;

				GetUpdateRect(DisplayWindow,&UpdateRect,FALSE);
				GetClientRect(DisplayWindow,&rc);

				hdc = BeginPaint(DisplayWindow,&ps);

				PatchBackground(hdc,&rc,&UpdateRect);

				EndPaint(DisplayWindow,&ps);
			}
			break;

		case DWM_BUFFERTEXT:
			{
				LineData_t ld;
				TCHAR *pszText = NULL;

				pszText = (TCHAR *)lParam;

				if(pszText != NULL)
				{
					StringCchCopy(ld.szText,SIZEOF_ARRAY(ld.szText),pszText);

					m_LineList.push_back(ld);
				}

				/* TODO: Optimize? */
				RedrawWindow(DisplayWindow,NULL,NULL,RDW_INVALIDATE);
			}
			break;

		case DWM_CLEARTEXTBUFFER:
			{
				m_LineList.clear();
			}
			break;

		case DWM_SETLINE:
			{
				TCHAR *pszText = NULL;
				std::vector<LineData_t>::iterator itr;
				unsigned int iLine;
				unsigned int i = 0;

				iLine = (int)wParam;
				pszText = (TCHAR *)lParam;

				if(pszText != NULL && iLine < m_LineList.size())
				{
					for(itr = m_LineList.begin();itr != m_LineList.end();itr++)
					{
						if(i == iLine)
						{
							StringCchCopy(itr->szText,SIZEOF_ARRAY(itr->szText),pszText);
							break;
						}

						i++;
					}
				}

				/* TODO: Optimize? */
				RedrawWindow(DisplayWindow,NULL,NULL,RDW_INVALIDATE);
			}
			break;

		case DWM_SETTHUMBNAILFILE:
			OnSetThumbnailFile(wParam,lParam);
			RedrawWindow(DisplayWindow,NULL,NULL,RDW_INVALIDATE);
			break;

		case DWM_GETCENTRECOLOR:
			return m_CentreColor.ToCOLORREF();
			break;

		case DWM_GETSURROUNDCOLOR:
			return m_SurroundColor.ToCOLORREF();
			break;

		case DWM_SETCENTRECOLOR:
			{
				m_CentreColor.SetFromCOLORREF((COLORREF)wParam);
				HDC hdc;
				hdc = GetDC(DisplayWindow);
				RECT rc;
				GetClientRect(DisplayWindow,&rc);
				DrawGradientFill(hdc,&rc);
				ReleaseDC(DisplayWindow,hdc);
				RedrawWindow(DisplayWindow,NULL,NULL,RDW_INVALIDATE);
			}
			break;

		case DWM_SETSURROUNDCOLOR:
			{
				m_SurroundColor.SetFromCOLORREF((COLORREF)wParam);
				HDC hdc;
				hdc = GetDC(DisplayWindow);
				RECT rc;
				GetClientRect(DisplayWindow,&rc);
				DrawGradientFill(hdc,&rc);
				ReleaseDC(DisplayWindow,hdc);
				RedrawWindow(DisplayWindow,NULL,NULL,RDW_INVALIDATE);
			}
			break;

		case DWM_GETFONT:
			HFONT *hFont;
			hFont = (HFONT *)wParam;
			*hFont = m_hDisplayFont;
			break;

		case DWM_SETFONT:
			OnSetFont((HFONT)wParam);
			break;

		case DWM_GETTEXTCOLOR:
			return m_TextColor;
			break;

		case DWM_SETTEXTCOLOR:
			OnSetTextColor((COLORREF)wParam);
			break;

		case DWM_DRAWGRADIENTFILL:
			PatchBackground((HDC)wParam,(RECT *)lParam,(RECT *)lParam);
			break;

		case WM_ERASEBKGND:
			HDC hdc;
			hdc = GetDC(DisplayWindow);
			RECT rc;
			RECT UpdateRect;
			GetUpdateRect(DisplayWindow,&UpdateRect,FALSE);
			GetClientRect(DisplayWindow,&rc);
			PatchBackground(hdc,&rc,&UpdateRect);
			ReleaseDC(DisplayWindow,hdc);
			return 1;
			break;

		case WM_SIZE:
			OnSize(LOWORD(lParam), HIWORD(lParam));
			break;

		case WM_USER_DISPLAYWINDOWMOVED:
			m_bVertical = (BOOL)wParam;
			InvalidateRect(m_hDisplayWindow, NULL, TRUE);
			break;
	}

	return DefWindowProc(DisplayWindow,msg,wParam,lParam);
}