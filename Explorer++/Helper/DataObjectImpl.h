// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WinRTBaseWrapper.h"
#include <shlobj.h>
#include <vector>

class DataObjectImpl :
	public winrt::implements<DataObjectImpl, IDataObject, IDataObjectAsyncCapability,
		winrt::non_agile>
{
public:
	DataObjectImpl(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, int count);
	~DataObjectImpl();

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
	struct DataObjectInternal
	{
		FORMATETC fe;
		STGMEDIUM stg;
	};

	BOOL DuplicateStorageMedium(STGMEDIUM *pstgDest, const STGMEDIUM *pstgSrc,
		const FORMATETC *pftc);
	BOOL DuplicateData(STGMEDIUM *pstgDest, const STGMEDIUM *pstgSrc, const FORMATETC *pftc);

	std::vector<DataObjectInternal> m_daoList;

	BOOL m_bInOperation;
	BOOL m_bDoOpAsync;
};
