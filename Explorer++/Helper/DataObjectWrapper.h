// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <objidl.h>
#include <shldisp.h>

// Wraps an existing IDataObject instance that doesn't support IDataObjectAsyncCapability.
class DataObjectWrapper : public IDataObject, public IDataObjectAsyncCapability
{
public:
	static wil::com_ptr_nothrow<DataObjectWrapper> Create(IDataObject *dataObject);

	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	// IDataObject
	IFACEMETHODIMP GetData(FORMATETC *formatEtc, STGMEDIUM *medium);
	IFACEMETHODIMP GetDataHere(FORMATETC *formatEtc, STGMEDIUM *medium);
	IFACEMETHODIMP QueryGetData(FORMATETC *formatEtc);
	IFACEMETHODIMP GetCanonicalFormatEtc(FORMATETC *formatEtc, FORMATETC *formatEtcResult);
	IFACEMETHODIMP SetData(FORMATETC *formatEtc, STGMEDIUM *medium, BOOL shouldRelease);
	IFACEMETHODIMP EnumFormatEtc(DWORD direction, IEnumFORMATETC **enumerator);
	IFACEMETHODIMP DAdvise(FORMATETC *formatEtc, DWORD advf, IAdviseSink *sink, DWORD *connection);
	IFACEMETHODIMP DUnadvise(DWORD connection);
	IFACEMETHODIMP EnumDAdvise(IEnumSTATDATA **enumerator);

	// IDataObjectAsyncCapability
	IFACEMETHODIMP GetAsyncMode(BOOL *isOpAsync);
	IFACEMETHODIMP SetAsyncMode(BOOL doOpAsync);
	IFACEMETHODIMP InOperation(BOOL *inAsyncOp);
	IFACEMETHODIMP StartOperation(IBindCtx *reserved);
	IFACEMETHODIMP EndOperation(HRESULT result, IBindCtx *reserved, DWORD effects);

private:
	DataObjectWrapper(IDataObject *dataObject);

	wil::com_ptr_nothrow<IDataObject> m_dataObject;
	ULONG m_refCount;

	BOOL m_inOperation;
	BOOL m_isOpAsync;
};