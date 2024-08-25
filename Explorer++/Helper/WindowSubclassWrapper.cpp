// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WindowSubclassWrapper.h"

WindowSubclassWrapper::WindowSubclassWrapper(HWND hwnd, Subclass subclass) :
	m_hwnd(hwnd),
	m_subclass(subclass),
	m_subclassId(m_subclassIdCounter++)
{
	m_subclassInstalled =
		SetWindowSubclass(hwnd, SubclassProcStub, m_subclassId, reinterpret_cast<DWORD_PTR>(this));
}

WindowSubclassWrapper::~WindowSubclassWrapper()
{
	if (m_subclassInstalled)
	{
		BOOL res = RemoveWindowSubclass(m_hwnd, SubclassProcStub, m_subclassId);
		DCHECK(res);
	}
}

LRESULT CALLBACK WindowSubclassWrapper::SubclassProcStub(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam, UINT_PTR subclassId, DWORD_PTR data)
{
	UNREFERENCED_PARAMETER(subclassId);

	auto *subclassWrapper = reinterpret_cast<WindowSubclassWrapper *>(data);

	if (msg == WM_NCDESTROY)
	{
		// There's no need to remove the subclass if the window is about to be destroyed.
		subclassWrapper->m_subclassInstalled = false;
	}

	return subclassWrapper->m_subclass(hwnd, msg, wParam, lParam);
}
