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
#include <list>

using namespace std;

/* TODO: */
extern HRESULT CreateEnumFormatEtc(list<FORMATETC> feList,IEnumFORMATETC **ppEnumFormatEtc);

struct DataObjectInternal
{
	FORMATETC	fe;
	STGMEDIUM	stg;
};

class CDataObject : public IDataObject, public IAsyncOperation
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

	/* IAsyncOperation. */
	HRESULT __stdcall	EndOperation(HRESULT hResult,IBindCtx *pbcReserved,DWORD dwEffects);
	HRESULT __stdcall	GetAsyncMode(BOOL *pfIsOpAsync);
	HRESULT __stdcall	InOperation(BOOL *pfInAsyncOp);
	HRESULT __stdcall	SetAsyncMode(BOOL fDoOpAsync);
	HRESULT __stdcall	StartOperation(IBindCtx *pbcReserved);

private:

	int							m_iRefCount;

	list<DataObjectInternal>	m_daoList;

	BOOL						m_bStartedOperation;
	BOOL						m_bDoOpAsync;
};

HRESULT CreateDataObject(FORMATETC *pFormatEtc,STGMEDIUM *pMedium,IDataObject **pDataObject,int count)
{
	*pDataObject = new CDataObject(pFormatEtc,pMedium,count);

	return S_OK;
}

CDataObject::CDataObject(FORMATETC *pFormatEtc,STGMEDIUM *pMedium,int count)
{
	m_iRefCount = 1;

	for(int i = 0;i < count;i++)
	{
		DataObjectInternal dao = {pFormatEtc[i],pMedium[i]};
		m_daoList.push_back(dao);
	}

	SetAsyncMode(FALSE);

	m_bStartedOperation = FALSE;
}

CDataObject::~CDataObject()
{

}

/* IUnknown interface members. */
HRESULT __stdcall CDataObject::QueryInterface(REFIID iid, void **ppvObject)
{
	if(ppvObject == NULL)
	{
		return E_POINTER;
	}

	*ppvObject = NULL;

	if(IsEqualIID(iid,IID_IDataObject) ||
		IsEqualIID(iid,IID_IUnknown))
	{
		*ppvObject = this;
	}
	else if(IsEqualIID(iid,IID_IAsyncOperation))
	{
		*ppvObject = static_cast<IAsyncOperation *>(this);
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
	SIZE_T AllocationSize;
	PVOID pGlobal;
	PVOID l_pGlobal;

	if(QueryGetData(pFormatEtc) == DV_E_FORMATETC)
	{
		return DV_E_FORMATETC;
	}

	for each(auto dao in m_daoList)
	{
		if(dao.fe.cfFormat == pFormatEtc->cfFormat &&
		   dao.fe.tymed & pFormatEtc->tymed &&
		   dao.fe.dwAspect == pFormatEtc->dwAspect)
		{
			pMedium->tymed			= dao.stg.tymed;
			pMedium->pUnkForRelease	= 0;

			switch(dao.fe.tymed)
			{
			case TYMED_HGLOBAL:
				AllocationSize = GlobalSize(dao.stg.hGlobal);
				l_pGlobal = GlobalLock(dao.stg.hGlobal);

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
	}

	return DV_E_FORMATETC;
}

HRESULT __stdcall CDataObject::GetDataHere(FORMATETC *pFormatEtc,STGMEDIUM *pMedium)
{
	return DV_E_TYMED;
}

HRESULT	__stdcall CDataObject::QueryGetData(FORMATETC *pFormatEtc)
{
	for each(auto dao in m_daoList)
	{
		if(dao.fe.cfFormat == pFormatEtc->cfFormat &&
		   dao.fe.tymed & pFormatEtc->tymed &&
		   dao.fe.dwAspect == pFormatEtc->dwAspect)
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
	DataObjectInternal dao = {*pFormatEtc,*pMedium};
	m_daoList.push_back(dao);

	return S_OK;
}

HRESULT __stdcall CDataObject::EnumFormatEtc(DWORD dwDirection,IEnumFORMATETC **ppEnumFormatEtc)
{
	if(dwDirection == DATADIR_GET)
	{
		list<FORMATETC> feList;

		for each(auto dao in m_daoList)
		{
			feList.push_back(dao.fe);
		}

		return CreateEnumFormatEtc(feList,ppEnumFormatEtc);
	}

	return E_NOTIMPL;
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

HRESULT __stdcall CDataObject::EndOperation(HRESULT hResult,IBindCtx *pbcReserved,DWORD dwEffects)
{
	return S_OK;
}

HRESULT __stdcall CDataObject::GetAsyncMode(BOOL *pfIsOpAsync)
{
	*pfIsOpAsync = m_bDoOpAsync;

	return S_OK;
}

HRESULT __stdcall CDataObject::InOperation(BOOL *pfInAsyncOp)
{
	if(m_bStartedOperation)
	{
		*pfInAsyncOp = VARIANT_TRUE;
	}

	*pfInAsyncOp = VARIANT_FALSE;

	return S_OK;
}

HRESULT __stdcall CDataObject::SetAsyncMode(BOOL fDoOpAsync)
{
	m_bDoOpAsync = fDoOpAsync;

	/* TODO: */
	if(fDoOpAsync)
	{
		AddRef();
	}

	return S_OK;
}

HRESULT __stdcall CDataObject::StartOperation(IBindCtx *pbcReserved)
{
	m_bStartedOperation = TRUE;

	return S_OK;
}