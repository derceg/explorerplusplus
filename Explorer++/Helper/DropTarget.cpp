// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DropTarget.h"

wil::com_ptr<DropTarget> DropTarget::Create(HWND hwnd, DropTargetInternal *dropTargetInternal)
{
	return wil::com_ptr<DropTarget>(new DropTarget(hwnd, dropTargetInternal));
}

DropTarget::DropTarget(HWND hwnd, DropTargetInternal *dropTargetInternal) :
	m_hwnd(hwnd),
	m_dropTargetInternal(dropTargetInternal),
	m_refCount(0),
	m_withinDrag(false)
{
	RegisterDragDrop(hwnd, this);

	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclassWrapper>(hwnd, WndProc, SUBCLASS_ID, 0));
}

LRESULT CALLBACK DropTarget::WndProc(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam, UINT_PTR subclassId, DWORD_PTR data)
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

IFACEMETHODIMP DropTarget::QueryInterface(REFIID iid, void **object)
{
	static const QITAB qit[] =
	{
		QITABENT(DropTarget, IDropTarget),
		{nullptr}
	};

	return QISearch(this, qit, iid, object);
}

IFACEMETHODIMP_(ULONG) DropTarget::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) DropTarget::Release()
{
	long refCount = InterlockedDecrement(&m_refCount);

	if (refCount == 0)
	{
		delete this;
	}

	return refCount;
}

IFACEMETHODIMP DropTarget::DragEnter(IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect)
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

IFACEMETHODIMP DropTarget::DragOver(DWORD keyState, POINTL ptl, DWORD *effect)
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

IFACEMETHODIMP DropTarget::DragLeave()
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

IFACEMETHODIMP DropTarget::Drop(IDataObject *dataObject, DWORD keyState, POINTL ptl, DWORD *effect)
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

IDropTargetHelper *DropTarget::GetDropTargetHelper()
{
	if (!m_dropTargetHelper)
	{
		CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_dropTargetHelper));
	}

	return m_dropTargetHelper;
}

bool DropTarget::IsWithinDrag() const
{
	return m_withinDrag;
}