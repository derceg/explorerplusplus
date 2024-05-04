// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WinRTBaseWrapper.h"
#include <wil/resource.h>
#include <shlobj.h>
#include <vector>

class DataObjectImpl :
	public winrt::implements<DataObjectImpl, IDataObject, IDataObjectAsyncCapability,
		winrt::non_agile>
{
public:
	DataObjectImpl();

	// IDataObject
	IFACEMETHODIMP GetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium);
	IFACEMETHODIMP GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pMedium);
	IFACEMETHODIMP QueryGetData(FORMATETC *pFormatEtc);
	IFACEMETHODIMP GetCanonicalFormatEtc(FORMATETC *pFormatEtcIn, FORMATETC *pFormatEtcOut);
	IFACEMETHODIMP SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, BOOL fRelease);
	IFACEMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatetc);
	IFACEMETHODIMP DAdvise(FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink,
		DWORD *pdwConnection);
	IFACEMETHODIMP DUnadvise(DWORD dwConnection);
	IFACEMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise);

	// IDataObjectAsyncCapability
	IFACEMETHODIMP EndOperation(HRESULT hResult, IBindCtx *pbcReserved, DWORD dwEffects);
	IFACEMETHODIMP GetAsyncMode(BOOL *pfIsOpAsync);
	IFACEMETHODIMP InOperation(BOOL *pfInAsyncOp);
	IFACEMETHODIMP SetAsyncMode(BOOL fDoOpAsync);
	IFACEMETHODIMP StartOperation(IBindCtx *pbcReserved);

private:
	struct ItemData
	{
		FORMATETC format;
		wil::unique_stg_medium stg;
	};

	wil::unique_stg_medium DuplicateStorageMedium(const STGMEDIUM *pstgSrc, const FORMATETC *pftc);

	std::vector<ItemData> m_items;

	BOOL m_bInOperation;
	BOOL m_bDoOpAsync;
};
