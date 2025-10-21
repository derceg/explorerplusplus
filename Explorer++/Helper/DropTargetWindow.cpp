// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DropTargetWindow.h"

DropTargetWindow::DropTargetWindow(HWND hwnd, DropTargetInternal *dropTargetInternal) :
	m_hwnd(hwnd),
	m_dropTargetInternal(dropTargetInternal)
{
	RegisterDragDrop(hwnd, this);

	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclass>(hwnd, std::bind_front(&DropTargetWindow::WndProc, this)));
}

LRESULT DropTargetWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		RevokeDragDrop(hwnd);
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

IFACEMETHODIMP DropTargetWindow::DragEnter(IDataObject *dataObject, DWORD keyState, POINTL ptl,
	DWORD *effect)
{
	POINT pt = { ptl.x, ptl.y };

	IDropTargetHelper *dropTargetHelper = GetDropTargetHelper();

	if (dropTargetHelper)
	{
		dropTargetHelper->DragEnter(m_hwnd, dataObject, &pt, *effect);
	}

	*effect = m_dropTargetInternal->DragEnter(dataObject, keyState, pt, *effect);

	return S_OK;
}

IFACEMETHODIMP DropTargetWindow::DragOver(DWORD keyState, POINTL ptl, DWORD *effect)
{
	POINT pt = { ptl.x, ptl.y };

	IDropTargetHelper *dropTargetHelper = GetDropTargetHelper();

	if (dropTargetHelper)
	{
		dropTargetHelper->DragOver(&pt, *effect);
	}

	*effect = m_dropTargetInternal->DragOver(keyState, pt, *effect);

	return S_OK;
}

IFACEMETHODIMP DropTargetWindow::DragLeave()
{
	IDropTargetHelper *dropTargetHelper = GetDropTargetHelper();

	if (dropTargetHelper)
	{
		dropTargetHelper->DragLeave();
	}

	m_dropTargetInternal->DragLeave();

	return S_OK;
}

IFACEMETHODIMP DropTargetWindow::Drop(IDataObject *dataObject, DWORD keyState, POINTL ptl,
	DWORD *effect)
{
	POINT pt = { ptl.x, ptl.y };

	IDropTargetHelper *dropTargetHelper = GetDropTargetHelper();

	if (dropTargetHelper)
	{
		dropTargetHelper->Drop(dataObject, &pt, *effect);
	}

	*effect = m_dropTargetInternal->Drop(dataObject, keyState, pt, *effect);

	return S_OK;
}

IDropTargetHelper *DropTargetWindow::GetDropTargetHelper()
{
	if (!m_dropTargetHelper)
	{
		CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_dropTargetHelper));
	}

	return m_dropTargetHelper;
}
