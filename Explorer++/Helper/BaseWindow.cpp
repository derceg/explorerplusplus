/******************************************************************
 *
 * Project: Helper
 * File: BaseWindow.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides a degree of abstraction off a standard window.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "BaseWindow.h"


CBaseWindow::CBaseWindow(HWND hwnd) :
CMessageForwarder()
{
	SetWindowSubclass(hwnd,BaseWindowProcStub,0,reinterpret_cast<DWORD_PTR>(this));
}

CBaseWindow::~CBaseWindow()
{
	
}

LRESULT CALLBACK BaseWindowProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,
	UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CBaseWindow *pbw = reinterpret_cast<CBaseWindow *>(dwRefData);

	return pbw->BaseWindowProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CBaseWindow::BaseWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd,BaseWindowProcStub,0);
		delete this;
		return 0;
		break;
	}

	return ForwardMessage(hwnd,uMsg,wParam,lParam);
}

INT_PTR CBaseWindow::GetDefaultReturnValue(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}