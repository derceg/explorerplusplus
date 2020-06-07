// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Macros.h"
#include <functional>

class WindowSubclassWrapper
{
public:
	using Subclass = std::function<LRESULT(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)>;

	WindowSubclassWrapper(
		HWND hwnd, SUBCLASSPROC subclassProc, UINT_PTR subclassId, DWORD_PTR data);
	WindowSubclassWrapper(HWND hwnd, Subclass subclass, UINT_PTR subclassId);
	~WindowSubclassWrapper();

private:
	DISALLOW_COPY_AND_ASSIGN(WindowSubclassWrapper);

	static LRESULT CALLBACK SubclassProcStub(
		HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR data);

	HWND m_hwnd;
	SUBCLASSPROC m_subclassProc;
	Subclass m_subclass;
	UINT_PTR m_subclassId;
	BOOL m_subclassInstalled;
};