// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Provides support for acting as a drop source.
 */

#include "stdafx.h"
#include "iDropSource.h"
#include <wil/common.h>

class DropSource : public IDropSource
{
public:
	DropSource();

	HRESULT __stdcall QueryInterface(REFIID iid, void **ppvObject);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	HRESULT _stdcall QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
	HRESULT _stdcall GiveFeedback(DWORD dwEffect);

private:
	LONG m_lRefCount;
};

HRESULT CreateDropSource(IDropSource **ppDropSource)
{
	if (ppDropSource == nullptr)
	{
		return E_FAIL;
	}

	*ppDropSource = new DropSource();

	return S_OK;
}

DropSource::DropSource()
{
	m_lRefCount = 1;
}

/* IUnknown interface members. */
HRESULT __stdcall DropSource::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = nullptr;

	if (iid == IID_IDropSource || iid == IID_IUnknown)
	{
		*ppvObject = this;
	}

	if (*ppvObject)
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

	if (lCount == 0)
	{
		delete this;
		return 0;
	}

	return lCount;
}

HRESULT _stdcall DropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	if (fEscapePressed == TRUE
		|| (WI_IsFlagSet(grfKeyState, MK_LBUTTON) && WI_IsFlagSet(grfKeyState, MK_RBUTTON)))
	{
		return DRAGDROP_S_CANCEL;
	}

	if (WI_IsFlagClear(grfKeyState, MK_LBUTTON) && WI_IsFlagClear(grfKeyState, MK_RBUTTON))
	{
		return DRAGDROP_S_DROP;
	}

	return S_OK;
}

HRESULT _stdcall DropSource::GiveFeedback(DWORD dwEffect)
{
	UNREFERENCED_PARAMETER(dwEffect);

	return DRAGDROP_S_USEDEFAULTCURSORS;
}
