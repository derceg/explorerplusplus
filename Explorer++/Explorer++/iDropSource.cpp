/******************************************************************
 *
 * Project: Explorer++
 * File: iDropSource.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides support for acting as a drop source.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "iDropSource.h"


class CDropSource : public IDropSource
{
public:

	CDropSource(DragTypes_t DragType);
	~CDropSource();

	HRESULT		__stdcall	QueryInterface(REFIID iid, void **ppvObject);
	ULONG		__stdcall	AddRef(void);
	ULONG		__stdcall	Release(void);

	HRESULT _stdcall	QueryContinueDrag(BOOL fEscapePressed,DWORD gfrKeyState);
	HRESULT _stdcall	GiveFeedback(DWORD dwEffect);

private:

	int			m_iRefCount;
	DragTypes_t	m_DragType;
};

HRESULT CreateDropSource(IDropSource **ppDropSource,DragTypes_t DragType)
{
	*ppDropSource = new CDropSource(DragType);

	return S_OK;
}

CDropSource::CDropSource(DragTypes_t DragType)
{
	m_iRefCount	= 1;
	m_DragType = DragType;
}

CDropSource::~CDropSource()
{

}

/* IUnknown interface members. */
HRESULT __stdcall CDropSource::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IDropSource ||
		iid == IID_IUnknown)
	{
		*ppvObject=this;
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CDropSource::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall CDropSource::Release(void)
{
	m_iRefCount--;

	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}


HRESULT _stdcall CDropSource::QueryContinueDrag(BOOL fEscapePressed,DWORD grfKeyState)
{
	DWORD dwStopButton = 0;

	if(m_DragType == DRAG_TYPE_LEFTCLICK)
	{
		if((grfKeyState & MK_LBUTTON) == 0)
			return DRAGDROP_S_DROP;

		dwStopButton = MK_RBUTTON;
	}
	else if(m_DragType == DRAG_TYPE_RIGHTCLICK)
	{
		if((grfKeyState & MK_RBUTTON) == 0)
			return DRAGDROP_S_DROP;

		dwStopButton = MK_LBUTTON;
	}

	if(fEscapePressed == TRUE || grfKeyState & dwStopButton)
		return DRAGDROP_S_CANCEL;

	return S_OK;
}

HRESULT _stdcall CDropSource::GiveFeedback(DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}