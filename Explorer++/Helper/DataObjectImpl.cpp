// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DataObjectImpl.h"
#include "EnumFormatEtcImpl.h"
#include <list>

DataObjectImpl::DataObjectImpl()
{
	SetAsyncMode(FALSE);

	m_bInOperation = FALSE;
}

// IDataObject
IFACEMETHODIMP DataObjectImpl::GetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
	if (pFormatEtc == nullptr || pMedium == nullptr)
	{
		return E_INVALIDARG;
	}

	for (const auto &item : m_items)
	{
		if (item.format.cfFormat == pFormatEtc->cfFormat && item.format.tymed & pFormatEtc->tymed
			&& item.format.dwAspect == pFormatEtc->dwAspect)
		{
			auto duplicatedStg = DuplicateStorageMedium(&item.stg, &item.format);
			*pMedium = duplicatedStg.release();

			return S_OK;
		}
	}

	return DV_E_FORMATETC;
}

wil::unique_stg_medium DataObjectImpl::DuplicateStorageMedium(const STGMEDIUM *pstgSrc,
	const FORMATETC *pftc)
{
	wil::unique_stg_medium duplicateStg;

	switch (pftc->tymed)
	{
	case TYMED_HGLOBAL:
		duplicateStg.hGlobal =
			static_cast<HGLOBAL>(OleDuplicateData(pstgSrc->hGlobal, pftc->cfFormat, 0));
		break;

	case TYMED_FILE:
		duplicateStg.lpszFileName =
			static_cast<LPOLESTR>(OleDuplicateData(pstgSrc->lpszFileName, pftc->cfFormat, 0));
		break;

	case TYMED_GDI:
		duplicateStg.hBitmap =
			static_cast<HBITMAP>(OleDuplicateData(pstgSrc->hBitmap, pftc->cfFormat, 0));
		break;

	case TYMED_MFPICT:
		duplicateStg.hMetaFilePict =
			static_cast<HMETAFILEPICT>(OleDuplicateData(pstgSrc->hMetaFilePict, pftc->cfFormat, 0));
		break;

	case TYMED_ENHMF:
		duplicateStg.hEnhMetaFile =
			static_cast<HENHMETAFILE>(OleDuplicateData(pstgSrc->hEnhMetaFile, pftc->cfFormat, 0));
		break;

	case TYMED_ISTREAM:
		duplicateStg.pstm = pstgSrc->pstm;
		pstgSrc->pstm->AddRef();
		break;

	case TYMED_ISTORAGE:
		duplicateStg.pstg = pstgSrc->pstg;
		pstgSrc->pstg->AddRef();
		break;

	case TYMED_NULL:
		/* Do nothing. */
		break;
	}

	duplicateStg.tymed = pstgSrc->tymed;
	duplicateStg.pUnkForRelease = pstgSrc->pUnkForRelease;

	if (duplicateStg.pUnkForRelease)
	{
		duplicateStg.pUnkForRelease->AddRef();
	}

	return duplicateStg;
}

IFACEMETHODIMP DataObjectImpl::GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
	UNREFERENCED_PARAMETER(pFormatEtc);
	UNREFERENCED_PARAMETER(pMedium);

	return DV_E_TYMED;
}

IFACEMETHODIMP DataObjectImpl::QueryGetData(FORMATETC *pFormatEtc)
{
	if (pFormatEtc == nullptr)
	{
		return E_INVALIDARG;
	}

	for (const auto &item : m_items)
	{
		if (item.format.cfFormat == pFormatEtc->cfFormat && item.format.tymed & pFormatEtc->tymed
			&& item.format.dwAspect == pFormatEtc->dwAspect)
		{
			return S_OK;
		}
	}

	return DV_E_FORMATETC;
}

IFACEMETHODIMP DataObjectImpl::GetCanonicalFormatEtc(FORMATETC *pFormatEtcIn,
	FORMATETC *pFormatEtcOut)
{
	UNREFERENCED_PARAMETER(pFormatEtcIn);

	if (pFormatEtcOut == nullptr)
	{
		return E_INVALIDARG;
	}

	pFormatEtcOut->ptd = nullptr;

	return E_NOTIMPL;
}

IFACEMETHODIMP DataObjectImpl::SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, BOOL fRelease)
{
	if (pFormatEtc == nullptr || pMedium == nullptr)
	{
		return E_INVALIDARG;
	}

	ItemData itemData;

	itemData.format = *pFormatEtc;

	if (fRelease)
	{
		itemData.stg.reset(*pMedium);
	}
	else
	{
		itemData.stg = DuplicateStorageMedium(pMedium, pFormatEtc);
	}

	m_items.push_back(std::move(itemData));

	return S_OK;
}

IFACEMETHODIMP DataObjectImpl::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
	if (ppEnumFormatEtc == nullptr)
	{
		return E_INVALIDARG;
	}

	if (dwDirection == DATADIR_GET)
	{
		std::list<FORMATETC> feList;

		for (const auto &item : m_items)
		{
			feList.push_back(item.format);
		}

		auto enumFormatEtcImpl = winrt::make_self<EnumFormatEtcImpl>(feList);
		*ppEnumFormatEtc = enumFormatEtcImpl.detach();

		return S_OK;
	}

	return E_NOTIMPL;
}

IFACEMETHODIMP DataObjectImpl::DAdvise(FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink,
	DWORD *pdwConnection)
{
	UNREFERENCED_PARAMETER(pFormatEtc);
	UNREFERENCED_PARAMETER(advf);
	UNREFERENCED_PARAMETER(pAdvSink);
	UNREFERENCED_PARAMETER(pdwConnection);

	return E_NOTIMPL;
}

IFACEMETHODIMP DataObjectImpl::DUnadvise(DWORD dwConnection)
{
	UNREFERENCED_PARAMETER(dwConnection);

	return OLE_E_ADVISENOTSUPPORTED;
}

IFACEMETHODIMP DataObjectImpl::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
	UNREFERENCED_PARAMETER(ppenumAdvise);

	return OLE_E_ADVISENOTSUPPORTED;
}

// IDataObjectAsyncCapability
// End operation does not seem to be called when dropping the CF_HDROP format into Windows Explorer.
// See: http://us.generation-nt.com/iasyncoperation-idataobject-help-45020022.html
IFACEMETHODIMP DataObjectImpl::EndOperation(HRESULT hResult, IBindCtx *pbcReserved, DWORD dwEffects)
{
	UNREFERENCED_PARAMETER(hResult);
	UNREFERENCED_PARAMETER(pbcReserved);
	UNREFERENCED_PARAMETER(dwEffects);

	m_bInOperation = FALSE;
	return S_OK;
}

IFACEMETHODIMP DataObjectImpl::GetAsyncMode(BOOL *pfIsOpAsync)
{
	*pfIsOpAsync = m_bDoOpAsync;

	return S_OK;
}

IFACEMETHODIMP DataObjectImpl::InOperation(BOOL *pfInAsyncOp)
{
	*pfInAsyncOp = m_bInOperation;

	return S_OK;
}

IFACEMETHODIMP DataObjectImpl::SetAsyncMode(BOOL fDoOpAsync)
{
	m_bDoOpAsync = fDoOpAsync;

	return S_OK;
}

IFACEMETHODIMP DataObjectImpl::StartOperation(IBindCtx *pbcReserved)
{
	UNREFERENCED_PARAMETER(pbcReserved);

	m_bInOperation = TRUE;

	return S_OK;
}
