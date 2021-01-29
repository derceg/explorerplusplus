// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DropTargetWindow.h"

wil::com_ptr_nothrow<DropTargetWindow> DropTargetWindow::Create(
	HWND hwnd, DropTargetInternal *dropTargetInternal)
{
	wil::com_ptr_nothrow<DropTargetWindow> dropTarget;
	dropTarget.attach(new DropTargetWindow(hwnd, dropTargetInternal));
	return dropTarget;
}

DropTargetWindow::DropTargetWindow(HWND hwnd, DropTargetInternal *dropTargetInternal) :
	m_hwnd(hwnd),
	m_dropTargetInternal(dropTargetInternal),
	m_refCount(1),
	m_withinDrag(false)
{
	RegisterDragDrop(hwnd, this);

	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclassWrapper>(hwnd, WndProc, SUBCLASS_ID, 0));
}

LRESULT CALLBACK DropTargetWindow::WndProc(
	HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR subclassId, DWORD_PTR data)
{
	UNREFERENCED_PARAMETER(subclassId);
	UNREFERENCED_PARAMETER(data);

	switch (msg)
	{
	case WM_DESTROY:
		RevokeDragDrop(hwnd);
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

IFACEMETHODIMP DropTargetWindow::QueryInterface(REFIID iid, void **object)
{
	static const QITAB qit[] = { QITABENT(DropTargetWindow, IDropTarget), { nullptr } };

	return QISearch(this, qit, iid, object);
}

IFACEMETHODIMP_(ULONG) DropTargetWindow::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) DropTargetWindow::Release()
{
	long refCount = InterlockedDecrement(&m_refCount);

	if (refCount == 0)
	{
		delete this;
	}

	return refCount;
}

IFACEMETHODIMP DropTargetWindow::DragEnter(
	IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect)
{
	POINT pt = { ptl.x, ptl.y };

	IDropTargetHelper *dropTargetHelper = GetDropTargetHelper();

	if (dropTargetHelper)
	{
		dropTargetHelper->DragEnter(m_hwnd, dataObject, &pt, *effect);
	}

	*effect = m_dropTargetInternal->DragEnter(dataObject, keyState, pt, *effect);

	m_withinDrag = true;

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

	m_withinDrag = false;

	return S_OK;
}

IFACEMETHODIMP DropTargetWindow::Drop(
	IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect)
{
	POINT pt = { ptl.x, ptl.y };

	IDropTargetHelper *dropTargetHelper = GetDropTargetHelper();

	if (dropTargetHelper)
	{
		dropTargetHelper->Drop(dataObject, &pt, *effect);
	}

	*effect = m_dropTargetInternal->Drop(dataObject, keyState, pt, *effect);

	m_withinDrag = false;

	return S_OK;
}

IDropTargetHelper *DropTargetWindow::GetDropTargetHelper()
{
	if (!m_dropTargetHelper)
	{
		CoCreateInstance(
			CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_dropTargetHelper));
	}

	return m_dropTargetHelper;
}

bool DropTargetWindow::IsWithinDrag() const
{
	return m_withinDrag;
}