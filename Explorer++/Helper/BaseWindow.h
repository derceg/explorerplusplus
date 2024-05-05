// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MessageForwarder.h"
#include <boost/core/noncopyable.hpp>

/* This lifetime of this object is
tied to its window. When the window
is destroyed, this object will
automatically delete itself.

Note that derived classes can
also safely delete the object
before the window is destroyed. */
class BaseWindow : public MessageForwarder, private boost::noncopyable
{
public:
	/* Ideally private, and the
	derived class would completely
	manage the window. */
	HWND GetHWND() const;

protected:
	BaseWindow(HWND hwnd);
	virtual ~BaseWindow();

	INT_PTR GetDefaultReturnValue(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	INT_PTR OnNcDestroy() final;

	const HWND m_hwnd;

private:
	static const UINT_PTR SUBCLASS_ID = 0;

	static LRESULT CALLBACK BaseWindowProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK BaseWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
