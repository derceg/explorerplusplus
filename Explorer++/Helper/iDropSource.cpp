// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Provides support for acting as a drop source.
 */

#include "stdafx.h"
#include "iDropSource.h"


class DropSource : public IDropSource
{
public:

	DropSource(DragType dragType);

	HRESULT		__stdcall	QueryInterface(REFIID iid, void **ppvObject);
	ULONG		__stdcall	AddRef();
	ULONG		__stdcall	Release();

	HRESULT _stdcall	QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
	HRESULT _stdcall	GiveFeedback(DWORD dwEffect);

private:

	LONG		m_lRefCount;
	DragType	m_DragType;
};

HRESULT CreateDropSource(IDropSource **ppDropSource,DragType dragType)
{
	if(ppDropSource == NULL)
	{
		return E_FAIL;
	}

	*ppDropSource = new DropSource(dragType);

	return S_OK;
}

DropSource::DropSource(DragType dragType)
{
	m_lRefCount = 1;
	m_DragType = dragType;
}

/* IUnknown interface members. */
HRESULT __stdcall DropSource::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IDropSource ||
		iid == IID_IUnknown)
	{
		*ppvObject = this;
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall DropSource::AddRef()
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG __stdcall DropSource::Release()
{
	LONG lCount = InterlockedDecrement(&m_lRefCount);

	if(lCount == 0)
	{
		delete this;
		return 0;
	}

	return lCount;
}


HRESULT _stdcall DropSource::QueryContinueDrag(BOOL fEscapePressed,DWORD grfKeyState)
{
	DWORD dwStopButton = 0;

	if(m_DragType == DragType::LeftClick)
	{
		if ((grfKeyState & MK_LBUTTON) == 0)
		{
			return DRAGDROP_S_DROP;
		}

		dwStopButton = MK_RBUTTON;
	}
	else if(m_DragType == DragType::RightClick)
	{
		if ((grfKeyState & MK_RBUTTON) == 0)
		{
			return DRAGDROP_S_DROP;
		}

		dwStopButton = MK_LBUTTON;
	}

	if (fEscapePressed == TRUE || grfKeyState & dwStopButton)
	{
		return DRAGDROP_S_CANCEL;
	}

	return S_OK;
}

HRESULT _stdcall DropSource::GiveFeedback(DWORD dwEffect)
{
	UNREFERENCED_PARAMETER(dwEffect);

	return DRAGDROP_S_USEDEFAULTCURSORS;
}