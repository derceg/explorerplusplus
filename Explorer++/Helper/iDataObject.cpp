// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include <list>
#include "iDataObject.h"
#include "iEnumFormatEtc.h"

struct DataObjectInternal
{
	FORMATETC	fe;
	STGMEDIUM	stg;
};

class CDataObject : public IDataObject, public IDataObjectAsyncCapability
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

	BOOL	DuplicateStorageMedium(STGMEDIUM *pstgDest, const STGMEDIUM *pstgSrc, const FORMATETC *pftc);
	BOOL	DuplicateData(STGMEDIUM *pstgDest, const STGMEDIUM *pstgSrc, const FORMATETC *pftc);

	LONG							m_lRefCount;

	std::list<DataObjectInternal>	m_daoList;

	BOOL							m_bInOperation;
	BOOL							m_bDoOpAsync;
};

IDataObject *CreateDataObject(FORMATETC *pFormatEtc,STGMEDIUM *pMedium,int count)
{
	return new CDataObject(pFormatEtc,pMedium,count);
}

CDataObject::CDataObject(FORMATETC *pFormatEtc,STGMEDIUM *pMedium,int count)
{
	m_lRefCount = 1;

	for(int i = 0;i < count;i++)
	{
		DataObjectInternal dao = {pFormatEtc[i],pMedium[i]};
		m_daoList.push_back(dao);
	}

	SetAsyncMode(FALSE);

	m_bInOperation = FALSE;
}

CDataObject::~CDataObject()
{
	for(auto dao : m_daoList)
	{
		ReleaseStgMedium(&dao.stg);
	}
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
	else if(IsEqualIID(iid, IID_IDataObjectAsyncCapability))
	{
		*ppvObject = static_cast<IDataObjectAsyncCapability *>(this);
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
	return InterlockedIncrement(&m_lRefCount);
}

ULONG __stdcall CDataObject::Release(void)
{
	LONG lCount = InterlockedDecrement(&m_lRefCount);

	if(lCount == 0)
	{
		delete this;
		return 0;
	}

	return lCount;
}

HRESULT __stdcall CDataObject::GetData(FORMATETC *pFormatEtc,STGMEDIUM *pMedium)
{
	if(pFormatEtc == NULL || pMedium == NULL)
	{
		return E_INVALIDARG;
	}

	if(QueryGetData(pFormatEtc) == DV_E_FORMATETC)
	{
		return DV_E_FORMATETC;
	}

	for(const auto &dao : m_daoList)
	{
		if(dao.fe.cfFormat == pFormatEtc->cfFormat &&
		   dao.fe.tymed & pFormatEtc->tymed &&
		   dao.fe.dwAspect == pFormatEtc->dwAspect)
		{
			BOOL bRet = DuplicateStorageMedium(pMedium,&dao.stg,&dao.fe);

			if(!bRet)
			{
				return STG_E_MEDIUMFULL;
			}

			return S_OK;
		}
	}

	return DV_E_FORMATETC;
}

BOOL CDataObject::DuplicateStorageMedium(STGMEDIUM *pstgDest, const STGMEDIUM *pstgSrc, const FORMATETC *pftc)
{
	pstgDest->tymed = pstgSrc->tymed;
	pstgDest->pUnkForRelease = NULL;

	if(pstgSrc->pUnkForRelease != NULL)
	{
		pstgDest->pUnkForRelease = pstgSrc->pUnkForRelease;
		pstgSrc->pUnkForRelease->AddRef();
	}

	BOOL success = TRUE;

	switch(pftc->tymed)
	{
	case TYMED_HGLOBAL:
	case TYMED_FILE:
	case TYMED_GDI:
	case TYMED_MFPICT:
	case TYMED_ENHMF:
		success = DuplicateData(pstgDest, pstgSrc, pftc);
		break;

	case TYMED_ISTREAM:
		pstgDest->pstm = pstgSrc->pstm;
		pstgSrc->pstm->AddRef();
		break;

	case TYMED_ISTORAGE:
		pstgDest->pstg = pstgSrc->pstg;
		pstgSrc->pstg->AddRef();
		break;

	case TYMED_NULL:
		/* Do nothing. */
		break;
	}

	return success;
}

BOOL CDataObject::DuplicateData(STGMEDIUM *pstgDest, const STGMEDIUM *pstgSrc, const FORMATETC *pftc)
{
	HANDLE hData = OleDuplicateData(pstgSrc->hGlobal,pftc->cfFormat,0);

	if(hData == NULL)
	{
		return FALSE;
	}

	switch(pftc->tymed)
	{
	case TYMED_HGLOBAL:
		pstgDest->hGlobal = hData;
		break;

	case TYMED_FILE:
		pstgDest->lpszFileName = reinterpret_cast<LPOLESTR>(hData);
		break;

	case TYMED_GDI:
		pstgDest->hBitmap = reinterpret_cast<HBITMAP>(hData);
		break;

	case TYMED_MFPICT:
		pstgDest->hMetaFilePict = hData;
		break;

	case TYMED_ENHMF:
		pstgDest->hEnhMetaFile = reinterpret_cast<HENHMETAFILE>(hData);
		break;
	}

	return TRUE;
}

HRESULT __stdcall CDataObject::GetDataHere(FORMATETC *pFormatEtc,STGMEDIUM *pMedium)
{
	UNREFERENCED_PARAMETER(pFormatEtc);
	UNREFERENCED_PARAMETER(pMedium);

	return DV_E_TYMED;
}

HRESULT	__stdcall CDataObject::QueryGetData(FORMATETC *pFormatEtc)
{
	if(pFormatEtc == NULL)
	{
		return E_INVALIDARG;
	}

	for(const auto &dao : m_daoList)
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
	UNREFERENCED_PARAMETER(pFormatEtcIn);

	if(pFormatEtcOut == NULL)
	{
		return E_INVALIDARG;
	}

	pFormatEtcOut->ptd = NULL;

	return E_NOTIMPL;
}

HRESULT __stdcall CDataObject::SetData(FORMATETC *pFormatEtc,STGMEDIUM *pMedium,BOOL fRelease)
{
	if(pFormatEtc == NULL || pMedium == NULL)
	{
		return E_INVALIDARG;
	}

	DataObjectInternal dao;

	dao.fe = *pFormatEtc;

	if(fRelease)
	{
		dao.stg = *pMedium;
	}
	else
	{
		BOOL bRet = DuplicateStorageMedium(&dao.stg,pMedium,pFormatEtc);

		if(!bRet)
		{
			return E_OUTOFMEMORY;
		}
	}

	m_daoList.push_back(dao);

	return S_OK;
}

HRESULT __stdcall CDataObject::EnumFormatEtc(DWORD dwDirection,IEnumFORMATETC **ppEnumFormatEtc)
{
	if(ppEnumFormatEtc == NULL)
	{
		return E_INVALIDARG;
	}

	if(dwDirection == DATADIR_GET)
	{
		std::list<FORMATETC> feList;

		for(const auto &dao : m_daoList)
		{
			feList.push_back(dao.fe);
		}

		return CreateEnumFormatEtc(feList,ppEnumFormatEtc);
	}

	return E_NOTIMPL;
}

HRESULT __stdcall CDataObject::DAdvise(FORMATETC *pFormatEtc,DWORD advf,IAdviseSink *pAdvSink,DWORD *pdwConnection)
{
	UNREFERENCED_PARAMETER(pFormatEtc);
	UNREFERENCED_PARAMETER(advf);
	UNREFERENCED_PARAMETER(pAdvSink);
	UNREFERENCED_PARAMETER(pdwConnection);

	return E_NOTIMPL;
}

HRESULT __stdcall CDataObject::DUnadvise(DWORD dwConnection)
{
	UNREFERENCED_PARAMETER(dwConnection);

	return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT __stdcall CDataObject::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
	UNREFERENCED_PARAMETER(ppenumAdvise);

	return OLE_E_ADVISENOTSUPPORTED;
}

/* End operation does not seem to be called
when dropping the CF_HDROP format into
Windows Explorer.
See: http://us.generation-nt.com/iasyncoperation-idataobject-help-45020022.html */
HRESULT __stdcall CDataObject::EndOperation(HRESULT hResult,
IBindCtx *pbcReserved,DWORD dwEffects)
{
	UNREFERENCED_PARAMETER(hResult);
	UNREFERENCED_PARAMETER(pbcReserved);
	UNREFERENCED_PARAMETER(dwEffects);

	m_bInOperation = FALSE;
	return S_OK;
}

HRESULT __stdcall CDataObject::GetAsyncMode(BOOL *pfIsOpAsync)
{
	*pfIsOpAsync = m_bDoOpAsync;

	return S_OK;
}

HRESULT __stdcall CDataObject::InOperation(BOOL *pfInAsyncOp)
{
	*pfInAsyncOp = m_bInOperation;

	return S_OK;
}

HRESULT __stdcall CDataObject::SetAsyncMode(BOOL fDoOpAsync)
{
	m_bDoOpAsync = fDoOpAsync;

	return S_OK;
}

HRESULT __stdcall CDataObject::StartOperation(IBindCtx *pbcReserved)
{
	UNREFERENCED_PARAMETER(pbcReserved);

	m_bInOperation = TRUE;

	return S_OK;
}