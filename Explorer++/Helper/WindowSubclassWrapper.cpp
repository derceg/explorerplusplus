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

WindowSubclassWrapper::WindowSubclassWrapper(WindowSubclassWrapper &&other) :
	m_hwnd(std::exchange(other.m_hwnd, nullptr)),
	m_subclassProc(std::exchange(other.m_subclassProc, nullptr)),
	m_subclassId(std::exchange(other.m_subclassId, 0)),
	m_subclassInstalled(std::exchange(other.m_subclassInstalled, FALSE))
{

}

WindowSubclassWrapper::~WindowSubclassWrapper()
{
	if (m_subclassInstalled)
	{
		RemoveWindowSubclass(m_hwnd, m_subclassProc, m_subclassId);
	}
}