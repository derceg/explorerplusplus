// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DataObjectImpl.h"
#include "EnumFormatEtcImpl.h"
#include <list>

// IDataObject
IFACEMETHODIMP DataObjectImpl::GetData(FORMATETC *format, STGMEDIUM *stg)
{
	if (format == nullptr || stg == nullptr)
	{
		return E_INVALIDARG;
	}

	for (const auto &item : m_items)
	{
		if (item.format.cfFormat == format->cfFormat && item.format.tymed & format->tymed
			&& item.format.dwAspect == format->dwAspect && item.format.lindex == format->lindex)
		{
			auto duplicatedStg = DuplicateStorageMedium(&item.stg, &item.format);
			*stg = duplicatedStg.release();

			return S_OK;
		}
	}

	return DV_E_FORMATETC;
}

wil::unique_stg_medium DataObjectImpl::DuplicateStorageMedium(const STGMEDIUM *stg,
	const FORMATETC *format)
{
	wil::unique_stg_medium duplicateStg;

	switch (format->tymed)
	{
	case TYMED_HGLOBAL:
		duplicateStg.hGlobal =
			static_cast<HGLOBAL>(OleDuplicateData(stg->hGlobal, format->cfFormat, 0));
		break;

	case TYMED_FILE:
		duplicateStg.lpszFileName =
			static_cast<LPOLESTR>(OleDuplicateData(stg->lpszFileName, format->cfFormat, 0));
		break;

	case TYMED_GDI:
		duplicateStg.hBitmap =
			static_cast<HBITMAP>(OleDuplicateData(stg->hBitmap, format->cfFormat, 0));
		break;

	case TYMED_MFPICT:
		duplicateStg.hMetaFilePict =
			static_cast<HMETAFILEPICT>(OleDuplicateData(stg->hMetaFilePict, format->cfFormat, 0));
		break;

	case TYMED_ENHMF:
		duplicateStg.hEnhMetaFile =
			static_cast<HENHMETAFILE>(OleDuplicateData(stg->hEnhMetaFile, format->cfFormat, 0));
		break;

	case TYMED_ISTREAM:
		duplicateStg.pstm = stg->pstm;
		stg->pstm->AddRef();
		break;

	case TYMED_ISTORAGE:
		duplicateStg.pstg = stg->pstg;
		stg->pstg->AddRef();
		break;

	case TYMED_NULL:
		/* Do nothing. */
		break;
	}

	duplicateStg.tymed = stg->tymed;
	duplicateStg.pUnkForRelease = stg->pUnkForRelease;

	if (duplicateStg.pUnkForRelease)
	{
		duplicateStg.pUnkForRelease->AddRef();
	}

	return duplicateStg;
}

IFACEMETHODIMP DataObjectImpl::GetDataHere(FORMATETC *format, STGMEDIUM *stg)
{
	UNREFERENCED_PARAMETER(format);
	UNREFERENCED_PARAMETER(stg);

	return DV_E_TYMED;
}

IFACEMETHODIMP DataObjectImpl::QueryGetData(FORMATETC *format)
{
	if (format == nullptr)
	{
		return E_INVALIDARG;
	}

	for (const auto &item : m_items)
	{
		if (item.format.cfFormat == format->cfFormat && item.format.tymed & format->tymed
			&& item.format.dwAspect == format->dwAspect)
		{
			return S_OK;
		}
	}

	return DV_E_FORMATETC;
}

IFACEMETHODIMP DataObjectImpl::GetCanonicalFormatEtc(FORMATETC *formatIn, FORMATETC *formatOut)
{
	UNREFERENCED_PARAMETER(formatIn);

	if (formatOut == nullptr)
	{
		return E_INVALIDARG;
	}

	formatOut->ptd = nullptr;

	return E_NOTIMPL;
}

IFACEMETHODIMP DataObjectImpl::SetData(FORMATETC *format, STGMEDIUM *stg, BOOL release)
{
	if (format == nullptr || stg == nullptr)
	{
		return E_INVALIDARG;
	}

	ItemData itemData;

	itemData.format = *format;

	if (release)
	{
		itemData.stg.reset(*stg);
	}
	else
	{
		itemData.stg = DuplicateStorageMedium(stg, format);
	}

	m_items.push_back(std::move(itemData));

	return S_OK;
}

IFACEMETHODIMP DataObjectImpl::EnumFormatEtc(DWORD direction, IEnumFORMATETC **enumFormatEtc)
{
	if (enumFormatEtc == nullptr)
	{
		return E_INVALIDARG;
	}

	if (direction == DATADIR_GET)
	{
		std::list<FORMATETC> feList;

		for (const auto &item : m_items)
		{
			feList.push_back(item.format);
		}

		auto enumFormatEtcImpl = winrt::make_self<EnumFormatEtcImpl>(feList);
		*enumFormatEtc = enumFormatEtcImpl.detach();

		return S_OK;
	}

	return E_NOTIMPL;
}

IFACEMETHODIMP DataObjectImpl::DAdvise(FORMATETC *format, DWORD flags, IAdviseSink *sink,
	DWORD *connection)
{
	UNREFERENCED_PARAMETER(format);
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(sink);
	UNREFERENCED_PARAMETER(connection);

	return OLE_E_ADVISENOTSUPPORTED;
}

IFACEMETHODIMP DataObjectImpl::DUnadvise(DWORD connection)
{
	UNREFERENCED_PARAMETER(connection);

	return OLE_E_ADVISENOTSUPPORTED;
}

IFACEMETHODIMP DataObjectImpl::EnumDAdvise(IEnumSTATDATA **enumAdvise)
{
	UNREFERENCED_PARAMETER(enumAdvise);

	return OLE_E_ADVISENOTSUPPORTED;
}

// IDataObjectAsyncCapability
// End operation does not seem to be called when dropping the CF_HDROP format into Windows Explorer.
// See: http://us.generation-nt.com/iasyncoperation-idataobject-help-45020022.html
IFACEMETHODIMP DataObjectImpl::EndOperation(HRESULT result, IBindCtx *reserved, DWORD effects)
{
	UNREFERENCED_PARAMETER(result);
	UNREFERENCED_PARAMETER(reserved);
	UNREFERENCED_PARAMETER(effects);

	m_inOperation = false;
	return S_OK;
}

IFACEMETHODIMP DataObjectImpl::GetAsyncMode(BOOL *isAsync)
{
	*isAsync = m_doOpAsync ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

IFACEMETHODIMP DataObjectImpl::InOperation(BOOL *inAsyncOp)
{
	*inAsyncOp = m_inOperation ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

IFACEMETHODIMP DataObjectImpl::SetAsyncMode(BOOL doOpAsync)
{
	m_doOpAsync = !!doOpAsync;

	return S_OK;
}

IFACEMETHODIMP DataObjectImpl::StartOperation(IBindCtx *reserved)
{
	UNREFERENCED_PARAMETER(reserved);

	m_inOperation = true;

	return S_OK;
}
