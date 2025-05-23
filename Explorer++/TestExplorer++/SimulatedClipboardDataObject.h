// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/WinRTBaseWrapper.h"
#include <wil/com.h>

// When simulating the clipboard, there are two cases that need to be handled:
//
// 1. When data is set using the equivalent of SetClipboardData().
// 2. When data is set using the equivalent of OleSetClipboard().
//
// In both cases, the simulated clipboard needs to manage an IDataObject instance. In the first
// case, the simulated clipboard will be responsible for creating the IDataObject instance, while it
// will be supplied externally in the second.
//
// Using a wrapper class in both cases makes sense, since calling SetData() on the IDataObject
// instance returned by OleGetClipboard() should always fail. This wrapper can enforce that
// invariant. Outside of SetData(), most other methods are simply forwarded to the delegate to
// handle. Therefore, this class is a basic wrapper around the supplied delegate.
class SimulatedClipboardDataObject :
	public winrt::implements<SimulatedClipboardDataObject, IDataObject, winrt::non_agile>
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

	// Custom methods
	wil::com_ptr_nothrow<IDataObject> GetDelegate() const;
	void SetDelegate(IDataObject *delegate);

private:
	wil::com_ptr_nothrow<IDataObject> m_delegate;
};
