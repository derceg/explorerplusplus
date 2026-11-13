// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ScopedDropTargetRegistration.h"
#include "WindowDropTarget.h"

ScopedDropTargetRegistration::ScopedDropTargetRegistration(HWND hwnd,
	DropTargetDelegate *delegate) :
	m_hwnd(hwnd)
{
	auto dropTarget = winrt::make_self<WindowDropTarget>(m_hwnd, delegate);
	HRESULT hr = RegisterDragDrop(m_hwnd, dropTarget.get());
	CHECK(SUCCEEDED(hr));
}

ScopedDropTargetRegistration::~ScopedDropTargetRegistration()
{
	HRESULT hr = RevokeDragDrop(m_hwnd);
	CHECK(SUCCEEDED(hr));
}
