// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WinRTBaseWrapper.h"
#include <wil/com.h>
#include <objidl.h>
#include <shldisp.h>

// Wraps an existing IDataObject instance that doesn't support IDataObjectAsyncCapability.
// Note the use of winrt::non_agile. That marker struct is necessary, otherwise copying and pasting
// within the application won't work.
// That is, if a file is copied from a tab or the treeview, then when it's pasted in another tab/the
// treeview, the operation will fail. The reason for that appears to be that the IMarshal
// implementation provided by C++/WinRT differs from the default implementation that's normally
// available.
// That then means that when the shell creates a new thread to perform the transfer (once it's
// detected that async operation is supported), the transfer fails on the background thread because
// a thread check fails (since the background thread isn't the thread the object was created on).
// Using the default implementation avoids that problem and that implementation is used when the
// object doesn't provide its own implementation.
class DataObjectWrapper :
	public winrt::implements<DataObjectWrapper, IDataObject, IDataObjectAsyncCapability,
		winrt::non_agile>
{
public:
	DataObjectWrapper(IDataObject *dataObject);

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
	wil::com_ptr_nothrow<IDataObject> m_dataObject;

	bool m_inOperation = false;
	bool m_isOpAsync = false;
};
