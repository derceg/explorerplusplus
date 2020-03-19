// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BaseWindow.h"


BaseWindow::BaseWindow(HWND hwnd) :
MessageForwarder(),
m_hwnd(hwnd)
{
	SetWindowSubclass(hwnd, BaseWindowProcStub, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));
}

BaseWindow::~BaseWindow()
{
	RemoveWindowSubclass(m_hwnd, BaseWindowProcStub, SUBCLASS_ID);
}

HWND BaseWindow::GetHWND() const
{
	return m_hwnd;
}

LRESULT CALLBACK BaseWindow::BaseWindowProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,
	UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pbw = reinterpret_cast<BaseWindow *>(dwRefData);

	return pbw->BaseWindowProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK BaseWindow::BaseWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return ForwardMessage(hwnd,uMsg,wParam,lParam);
}

INT_PTR BaseWindow::GetDefaultReturnValue(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

INT_PTR BaseWindow::OnNcDestroy()
{
	delete this;
	return 0;
}