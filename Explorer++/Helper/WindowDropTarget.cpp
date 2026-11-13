// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WindowDropTarget.h"
#include "DropTargetDelegate.h"

WindowDropTarget::WindowDropTarget(HWND hwnd, DropTargetDelegate *delegate) :
	m_hwnd(hwnd),
	m_delegate(delegate)
{
}

IFACEMETHODIMP WindowDropTarget::DragEnter(IDataObject *dataObject, DWORD keyState, POINTL ptl,
	DWORD *effect)
{
	POINT pt = { ptl.x, ptl.y };

	if (auto *dropTargetHelper = GetDropTargetHelper())
	{
		dropTargetHelper->DragEnter(m_hwnd, dataObject, &pt, *effect);
	}

	*effect = m_delegate->DragEnter(dataObject, keyState, pt, *effect);

	return S_OK;
}

IFACEMETHODIMP WindowDropTarget::DragOver(DWORD keyState, POINTL ptl, DWORD *effect)
{
	POINT pt = { ptl.x, ptl.y };

	if (auto *dropTargetHelper = GetDropTargetHelper())
	{
		dropTargetHelper->DragOver(&pt, *effect);
	}

	*effect = m_delegate->DragOver(keyState, pt, *effect);

	return S_OK;
}

IFACEMETHODIMP WindowDropTarget::DragLeave()
{
	if (auto *dropTargetHelper = GetDropTargetHelper())
	{
		dropTargetHelper->DragLeave();
	}

	m_delegate->DragLeave();

	return S_OK;
}

IFACEMETHODIMP WindowDropTarget::Drop(IDataObject *dataObject, DWORD keyState, POINTL ptl,
	DWORD *effect)
{
	POINT pt = { ptl.x, ptl.y };

	if (auto *dropTargetHelper = GetDropTargetHelper())
	{
		dropTargetHelper->Drop(dataObject, &pt, *effect);
	}

	*effect = m_delegate->Drop(dataObject, keyState, pt, *effect);

	return S_OK;
}

IDropTargetHelper *WindowDropTarget::GetDropTargetHelper()
{
	if (!m_dropTargetHelper)
	{
		CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_dropTargetHelper));
	}

	return m_dropTargetHelper;
}
