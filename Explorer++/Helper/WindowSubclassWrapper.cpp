// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WindowSubclassWrapper.h"

WindowSubclassWrapper::WindowSubclassWrapper(HWND hwnd, SUBCLASSPROC subclassProc,
	UINT_PTR subclassId, DWORD_PTR data) :
	m_hwnd(hwnd),
	m_subclassProc(subclassProc),
	m_subclassId(subclassId)
{
	m_subclassInstalled = SetWindowSubclass(hwnd, subclassProc, subclassId, data);
}

WindowSubclassWrapper::WindowSubclassWrapper(
	HWND hwnd, Subclass subclass, UINT_PTR subclassId) :
	m_hwnd(hwnd),
	m_subclassProc(SubclassProcStub),
	m_subclass(subclass),
	m_subclassId(subclassId)
{
	m_subclassInstalled = SetWindowSubclass(hwnd, SubclassProcStub, subclassId, reinterpret_cast<DWORD_PTR>(this));
}

WindowSubclassWrapper::~WindowSubclassWrapper()
{
	if (m_subclassInstalled)
	{
		RemoveWindowSubclass(m_hwnd, m_subclassProc, m_subclassId);
	}
}

LRESULT CALLBACK WindowSubclassWrapper::SubclassProcStub(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR data)
{
	UNREFERENCED_PARAMETER(subclassId);

	auto *subclassWrapper = reinterpret_cast<WindowSubclassWrapper *>(data);

	return subclassWrapper->m_subclass(hwnd, msg, wParam, lParam);
}