/******************************************************************
 *
 * Project: Helper
 * File: IDataObject.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides an implementation of IDataObject.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <string>

extern HRESULT CreateEnumFormatEtc(FORMATETC *pFormatEtc,IEnumFORMATETC **ppEnumFormatEtc,int count);

class CDataObject : public IDataObject
{
public:
	CDataObject(FORMATETC *,STGMEDIUM *,int);
	~CDataObject();

	HRESULT		__stdcall	QueryInterface(REFIID iid, void **ppvObject);
	ULONG		__stdcall	AddRef(void);
	ULONG		__stdcall	Release(void);

	HRESULT		__stdcall	GetData(FORMATETC *pFormatEtc,STGMEDIUM *pMedium);
	HRESULT		__stdcall	GetDataHere(FORMATETC *pFormatEtc,STGMEDIUM *pMedium);
	HRESULT		__stdcall	QueryGetData(FORMATETC *pFormatEtc);
	HRESULT		__stdcall	GetCanonicalFormatEtc(FORMATETC *pFormatEtcIn,FORMATETC *pFormatEtcOut);
	HRESULT		__stdcall	SetData(FORMATETC *pFormatEtc,STGMEDIUM *pMedium,BOOL fRelease);
	HRESULT		__stdcall	EnumFormatEtc(DWORD dwDirection,IEnumFORMATETC **ppEnumFormatetc);
	HRESULT		__stdcall	DAdvise(FORMATETC *pFormatEtc,DWORD advf,IAdviseSink *pAdvSink,DWORD *pdwConnection);
	HRESULT		__stdcall	DUnadvise(DWORD dwConnection);
	HRESULT		__stdcall	EnumDAdvise(IEnumSTATDATA **ppenumAdvise);

private:
	int m_iRefCount;

	FORMATETC *m_pFormatEtc;
	STGMEDIUM *m_pMedium;
	int m_NumFormatsStored;
};

HRESULT CreateDataObject(FORMATETC *pFormatEtc,STGMEDIUM *pMedium,IDataObject **pDataObject,int count)
{
	*pDataObject = new CDataObject(pFormatEtc,pMedium,count);

	return S_OK;
}

CDataObject::CDataObject(FORMATETC *pFormatEtc,STGMEDIUM *pMedium,int count)
{
	int i = 0;

	m_iRefCount		= 1;

	m_pFormatEtc	= new FORMATETC[count];
	m_pMedium		= new STGMEDIUM[count];

	for(i = 0;i  < count;i++)
	{
		m_pFormatEtc[i] = pFormatEtc[i];
		m_pMedium[i] = pMedium[i];
	}

	m_NumFormatsStored = count;
}

CDataObject::~CDataObject()
{

}

/* IUnknown interface members. */
HRESULT __stdcall CDataObject::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IDataObject || iid == IID_IUnknown)
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

ULONG __stdcall CDataObject::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall CDataObject::Release(void)
{
	m_iRefCount--;

	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}


HRESULT __stdcall CDataObject::GetData(FORMATETC *pFormatEtc,STGMEDIUM *pMedium)
{
	int i = 0;
	SIZE_T AllocationSize;
	PVOID pGlobal;
	PVOID l_pGlobal;

	if(QueryGetData(pFormatEtc) == DV_E_FORMATETC)
	{
		return DV_E_FORMATETC;
	}

	for(i = 0;i < m_NumFormatsStored;i++)
	{
		if((m_pFormatEtc[i].cfFormat == pFormatEtc->cfFormat) &&
		   (m_pFormatEtc[i].tymed & pFormatEtc->tymed) &&
		   (m_pFormatEtc[i].dwAspect == pFormatEtc->dwAspect))
		{
			break;
		}
	}

	pMedium->tymed			= m_pMedium[i].tymed;
	pMedium->pUnkForRelease	= 0;

	switch(m_pFormatEtc[i].tymed)
	{
		case TYMED_HGLOBAL:
			AllocationSize = GlobalSize(m_pMedium[i].hGlobal);
			l_pGlobal = GlobalLock(m_pMedium[i].hGlobal);

			pGlobal = GlobalAlloc(GMEM_FIXED,AllocationSize);

			if(pGlobal == NULL)
				return STG_E_MEDIUMFULL;

			memcpy(pGlobal,l_pGlobal,AllocationSize);

			GlobalUnlock(l_pGlobal);
			pMedium->hGlobal = pGlobal;
			break;

		default:
			return DV_E_FORMATETC;
			break;
	}

	return S_OK;
}

HRESULT __stdcall CDataObject::GetDataHere(FORMATETC *pFormatEtc,STGMEDIUM *pMedium)
{
	return DV_E_TYMED;
}

HRESULT	__stdcall CDataObject::QueryGetData(FORMATETC *pFormatEtc)
{
	int i = 0;

	for(i = 0;i < m_NumFormatsStored;i++)
	{
		if((m_pFormatEtc[i].cfFormat == pFormatEtc->cfFormat) &&
		   (m_pFormatEtc[i].tymed & pFormatEtc->tymed) &&
		   (m_pFormatEtc[i].dwAspect == pFormatEtc->dwAspect))
		{
			return S_OK;
		}
	}

	return DV_E_FORMATETC;
}

HRESULT __stdcall CDataObject::GetCanonicalFormatEtc(FORMATETC *pFormatEtcIn,FORMATETC *pFormatEtcOut)
{
	pFormatEtcOut->ptd = NULL;

	return E_NOTIMPL;
}

HRESULT __stdcall CDataObject::SetData(FORMATETC *pFormatEtc,STGMEDIUM *pMedium,BOOL fRelease)
{
	m_pFormatEtc = (FORMATETC *)realloc(m_pFormatEtc,(m_NumFormatsStored + 1) * sizeof(FORMATETC));
	m_pMedium = (STGMEDIUM *)realloc(m_pMedium,(m_NumFormatsStored + 1) * sizeof(STGMEDIUM));

	if(m_pFormatEtc == NULL || m_pMedium == NULL)
		return S_FALSE;

	m_pFormatEtc[m_NumFormatsStored]	= *pFormatEtc;
	m_pMedium[m_NumFormatsStored]		= *pMedium;

	m_NumFormatsStored++;

	return S_OK;
}

HRESULT __stdcall CDataObject::EnumFormatEtc(DWORD dwDirection,IEnumFORMATETC **ppEnumFormatEtc)
{
	if(dwDirection == DATADIR_GET)
	{
		return CreateEnumFormatEtc(m_pFormatEtc,ppEnumFormatEtc,m_NumFormatsStored);
	}
	else
	{
		return E_NOTIMPL;
	}
}

HRESULT __stdcall CDataObject::DAdvise(FORMATETC *pFormatEtc,DWORD advf,IAdviseSink *pAdvSink,DWORD *pdwConnection)
{
	return E_NOTIMPL;
}

HRESULT __stdcall CDataObject::DUnadvise(DWORD dwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT __stdcall CDataObject::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
	return OLE_E_ADVISENOTSUPPORTED;
}