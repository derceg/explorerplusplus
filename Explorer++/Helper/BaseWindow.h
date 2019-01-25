// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MessageForwarder.h"
#include "Macros.h"

/* This lifetime of this object is
tied to its window. When the window
is destroyed, this object will
automatically delete itself.

Note that derived classes can
also safely delete the object
before the window is destroyed. */
class CBaseWindow : public CMessageForwarder
{
	friend LRESULT CALLBACK BaseWindowProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

public:

	/* Ideally private, and the
	derived class would completely
	manage the window. */
	HWND GetHWND() const;

protected:

	CBaseWindow(HWND hwnd);
	virtual ~CBaseWindow();

	INT_PTR	GetDefaultReturnValue(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	virtual INT_PTR OnNcDestroy() final;

	const HWND m_hwnd;

private:

	DISALLOW_COPY_AND_ASSIGN(CBaseWindow);

	static const UINT_PTR SUBCLASS_ID = 0;

	LRESULT CALLBACK BaseWindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};