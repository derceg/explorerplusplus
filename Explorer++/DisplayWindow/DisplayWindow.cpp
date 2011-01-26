/******************************************************************
 *
 * Project: DisplayWindow
 * File: DisplayWindow.cpp
 * License: GPL - See COPYING in the top level directory
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
#include "DisplayWindowInternal.h"
#include "../Helper/Helper.h"
#include "../Helper/RegistrySettings.h"


using namespace Gdiplus;

#define CLASS_NAME				_T("DisplayWindow")
#define WINDOW_NAME				_T("DisplayWindow")

/* Private helper functions. */
LRESULT CALLBACK	DisplayWindowProcStub(HWND,UINT,WPARAM,LPARAM);
int					RegisterDisplayWindow(void);

ULONG_PTR	token;

BOOL RegisterDisplayWindow(void)
{
	WNDCLASS			wc;
	GdiplusStartupInput	startupInput;

	wc.style			= 0;
	wc.lpfnWndProc		= DisplayWindowProcStub;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= sizeof(CDisplayWindow *);
	wc.hInstance		= GetModuleHandle(0);
	wc.hIcon			= NULL;
	wc.hCursor			= LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)NULL;
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= CLASS_NAME;

	if(!RegisterClass(&wc))
		return FALSE;

	GdiplusStartup(&token,&startupInput,NULL);

	return TRUE;
}

HWND CreateDisplayWindow(HWND Parent,IDisplayWindowMain **pMain,
DWInitialSettings_t *pSettings)
{
	HWND hDisplayWindow;

	/* Registers the class needed to create the display window. */
	RegisterDisplayWindow();

	hDisplayWindow = CreateWindow(WINDOW_NAME,EMPTY_STRING,
	WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS,0,0,0,0,
	Parent,NULL,GetModuleHandle(0),(LPVOID)pSettings);

	*pMain = NULL;

	return hDisplayWindow;
}

/* IUnknown interface members. */
HRESULT __stdcall CDisplayWindow::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown *>(this);
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CDisplayWindow::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall CDisplayWindow::Release(void)
{
	m_iRefCount--;
	
	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

CDisplayWindow::CDisplayWindow(HWND hDisplayWindow,
DWInitialSettings_t *pInitialSettings)
{
	m_iRefCount = 1;

	g_ObjectCount++;

	m_hDisplayWindow = hDisplayWindow;

	m_hMainIcon = pInitialSettings->hIcon;

	m_LineSpacing	= 20;
	m_LeftIndent	= 80;

	m_bSizing		= FALSE;

	m_CentreColor	= pInitialSettings->CentreColor;
	m_SurroundColor	= pInitialSettings->SurroundColor;
	m_TextColor		= pInitialSettings->TextColor;
	m_hDisplayFont	= pInitialSettings->hFont;

	m_hbmThumbnail = NULL;
	m_bShowThumbnail = FALSE;
	m_bThumbnailExtracted = FALSE;
	m_bThumbnailExtractionFailed = FALSE;

	InitializeCriticalSection(&m_csDWThumbnails);
}

CDisplayWindow::~CDisplayWindow()
{
	DeleteDC(m_hdcBackground);
	DeleteObject(m_hBitmapBackground);

	DestroyIcon(m_hMainIcon);

	g_ObjectCount--;

	if(g_ObjectCount == 0)
		GdiplusShutdown(token);
}

LRESULT CALLBACK DisplayWindowProcStub(HWND DisplayWindow,UINT msg,
WPARAM wParam,LPARAM lParam)
{
	CDisplayWindow *pdw = (CDisplayWindow *)GetWindowLongPtr(DisplayWindow,GWLP_USERDATA);

	switch(msg)
	{
		case WM_CREATE:
			{
				CREATESTRUCT		*pCreate = NULL;
				DWInitialSettings_t	*pSettings = NULL;

				pCreate = (CREATESTRUCT *)lParam;

				pSettings = (DWInitialSettings_t *)pCreate->lpCreateParams;

				pdw = new CDisplayWindow(DisplayWindow,pSettings);
				SetWindowLongPtr(DisplayWindow,GWLP_USERDATA,(LONG_PTR)pdw);
			}
			break;

		case WM_DESTROY:
			/* TODO: This may cause an error (critical section freed
			while in use?). */
			//delete pdw;
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
				vector<LineData_t>::iterator itr;
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
				DrawGradientFill(hdc,&rc,&rc);
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
				DrawGradientFill(hdc,&rc,&rc);
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
			OnSize(wParam,lParam);
			break;

		case WM_DESTROY:
			//OnDestroyWindow();
			break;
	}

	return DefWindowProc(DisplayWindow,msg,wParam,lParam);
}