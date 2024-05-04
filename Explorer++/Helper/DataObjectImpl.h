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
	// IDataObject
	IFACEMETHODIMP GetData(FORMATETC *format, STGMEDIUM *stg);
	IFACEMETHODIMP GetDataHere(FORMATETC *format, STGMEDIUM *stg);
	IFACEMETHODIMP QueryGetData(FORMATETC *format);
	IFACEMETHODIMP GetCanonicalFormatEtc(FORMATETC *formatIn, FORMATETC *formatOut);
	IFACEMETHODIMP SetData(FORMATETC *format, STGMEDIUM *stg, BOOL release);
	IFACEMETHODIMP EnumFormatEtc(DWORD direction, IEnumFORMATETC **enumFormatEtc);
	IFACEMETHODIMP DAdvise(FORMATETC *format, DWORD flags, IAdviseSink *sink, DWORD *connection);
	IFACEMETHODIMP DUnadvise(DWORD connection);
	IFACEMETHODIMP EnumDAdvise(IEnumSTATDATA **enumAdvise);

	// IDataObjectAsyncCapability
	IFACEMETHODIMP EndOperation(HRESULT result, IBindCtx *reserved, DWORD effects);
	IFACEMETHODIMP GetAsyncMode(BOOL *isAsync);
	IFACEMETHODIMP InOperation(BOOL *inAsyncOp);
	IFACEMETHODIMP SetAsyncMode(BOOL doOpAsync);
	IFACEMETHODIMP StartOperation(IBindCtx *reserved);

private:
	struct ItemData
	{
		FORMATETC format;
		wil::unique_stg_medium stg;
	};

	wil::unique_stg_medium DuplicateStorageMedium(const STGMEDIUM *stg, const FORMATETC *format);

	std::vector<ItemData> m_items;

	bool m_inOperation = false;
	bool m_doOpAsync = false;
};
