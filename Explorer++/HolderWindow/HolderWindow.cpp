/******************************************************************
 *
 * Project: HolderWindow
 * File: HolderWindow.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Manages the 'holder window'. This window acts as a generic
 * container for child windows.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "HolderWindow.h"
#include "HolderWindowInternal.h"


#define FOLDERS_TEXT_X	5
#define FOLDERS_TEXT_Y	2

#define HOLDER_CLASS_NAME	_T("Holder")

ATOM				RegisterHolderWindowClass(void);
LRESULT CALLBACK	HolderWndProcStub(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

CHolderWindow::CHolderWindow(HWND hHolder)
{
	m_hHolder = hHolder;
	m_bHolderResizing	= FALSE;

	OSVERSIONINFO VersionInfo;

	VersionInfo.dwOSVersionInfoSize	= sizeof(OSVERSIONINFO);

	/* Need the OS version (for retrieving the nonclient
	text used to draw the window header). See the documentation
	for NONCLIENTMETRICS. ONLY NEEDED WHEN COMPILING FOR
	VISTA OR LATER (i.e. WINVER >= 0x0600). */
	if(GetVersionEx(&VersionInfo) != 0)
	{
		m_dwMajorVersion = VersionInfo.dwMajorVersion;
	}
}

CHolderWindow::~CHolderWindow()
{
	m_bHolderResizing	= FALSE;
}

ATOM RegisterHolderWindowClass(void)
{
	WNDCLASS wc;

	wc.style			= 0;
	wc.lpfnWndProc		= HolderWndProcStub;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= sizeof(CHolderWindow *);
	wc.hInstance		= GetModuleHandle(0);
	wc.hIcon			= NULL;
	wc.hCursor			= LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= HOLDER_CLASS_NAME;

	return RegisterClass(&wc);
}

HWND CreateHolderWindow(HWND hParent,TCHAR *szWindowName,UINT uStyle)
{
	HWND hHolder;

	RegisterHolderWindowClass();

	hHolder = CreateWindowEx(0,HOLDER_CLASS_NAME,szWindowName,
		uStyle,0,0,0,0,hParent,NULL,GetModuleHandle(0),NULL);

	return hHolder;
}

LRESULT CALLBACK HolderWndProcStub(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	CHolderWindow *pHolderWindow = (CHolderWindow *)GetWindowLongPtr(hwnd,GWLP_USERDATA);

	switch(msg)
	{
		case WM_CREATE:
			{
				pHolderWindow = new CHolderWindow(hwnd);

				SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)pHolderWindow);
			}
			break;

		case WM_DESTROY:
			delete pHolderWindow;
			return 0;
			break;
	}

	return pHolderWindow->HolderWndProc(hwnd,msg,wParam,lParam);
}

LRESULT CALLBACK CHolderWindow::HolderWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_LBUTTONDOWN:
			OnHolderWindowLButtonDown(lParam);
			break;

		case WM_LBUTTONUP:
			OnHolderWindowLButtonUp();
			break;

		case WM_MOUSEMOVE:
			return OnHolderWindowMouseMove(lParam);
			break;

		case WM_PAINT:
			OnHolderWindowPaint(hwnd);
			break;
	}

	return DefWindowProc(hwnd,msg,wParam,lParam);
}

/*
 * Draws the window text onto the holder
 * window.
 */
void CHolderWindow::OnHolderWindowPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	NONCLIENTMETRICS ncm;
	HDC hdc;
	HFONT hFont;
	RECT rc;
	TCHAR szHeader[64];

	GetClientRect(hwnd,&rc);

	hdc = BeginPaint(hwnd,&ps);

	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS),(PVOID)&ncm,0);
	ncm.lfSmCaptionFont.lfWeight = FW_NORMAL;
	hFont = CreateFontIndirect(&ncm.lfSmCaptionFont);

	SelectObject(hdc,hFont);

	GetWindowText(hwnd,szHeader,SIZEOF_ARRAY(szHeader));

	SetBkMode(hdc,TRANSPARENT);
	TextOut(hdc,FOLDERS_TEXT_X,FOLDERS_TEXT_Y,szHeader,lstrlen(szHeader));

	DeleteObject(hFont);

	EndPaint(hwnd,&ps);
}

void CHolderWindow::OnHolderWindowLButtonDown(LPARAM lParam)
{
	POINTS CursorPos;
	RECT rc;

	CursorPos = MAKEPOINTS(lParam);
	GetClientRect(m_hHolder,&rc);

	if(CursorPos.x >= (rc.right - 10))
	{
		SetCursor(LoadCursor(NULL,IDC_SIZEWE));

		m_bHolderResizing = TRUE;

		SetFocus(m_hHolder);
		SetCapture(m_hHolder);
	}
}

void CHolderWindow::OnHolderWindowLButtonUp(void)
{
	m_bHolderResizing = FALSE;

	ReleaseCapture();
}

int CHolderWindow::OnHolderWindowMouseMove(LPARAM lParam)
{
	static POINTS	ptsPrevCursor;
	POINTS			ptsCursor;
	RECT			rc;

	ptsCursor = MAKEPOINTS(lParam);
	GetClientRect(m_hHolder,&rc);

	/* Is the window in the process of been resized? */
	if(m_bHolderResizing)
	{
		/* Mouse hasn't moved. */
		if((ptsPrevCursor.x == ptsCursor.x)
			&& (ptsPrevCursor.y == ptsCursor.y))
			return 0;

		ptsPrevCursor.x = ptsCursor.x;
		ptsPrevCursor.y = ptsCursor.y;

		SendMessage(GetParent(m_hHolder),WM_USER_HOLDERRESIZED,(WPARAM)m_hHolder,(LPARAM)ptsCursor.x);

		return 1;
	}

	if(ptsCursor.x >= (rc.right - 10))
	{
		SetCursor(LoadCursor(NULL,IDC_SIZEWE));
	}

	return 0;
}