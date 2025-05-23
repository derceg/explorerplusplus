// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedClipboardDataObject.h"

IFACEMETHODIMP SimulatedClipboardDataObject::GetData(FORMATETC *format, STGMEDIUM *stg)
{
	return m_delegate ? m_delegate->GetData(format, stg) : E_NOTIMPL;
}

IFACEMETHODIMP SimulatedClipboardDataObject::GetDataHere(FORMATETC *format, STGMEDIUM *stg)
{
	return m_delegate ? m_delegate->GetDataHere(format, stg) : E_NOTIMPL;
}

IFACEMETHODIMP SimulatedClipboardDataObject::QueryGetData(FORMATETC *format)
{
	return m_delegate ? m_delegate->QueryGetData(format) : E_NOTIMPL;
}

IFACEMETHODIMP SimulatedClipboardDataObject::GetCanonicalFormatEtc(FORMATETC *formatIn,
	FORMATETC *formatOut)
{
	return m_delegate ? m_delegate->GetCanonicalFormatEtc(formatIn, formatOut) : E_NOTIMPL;
}

IFACEMETHODIMP SimulatedClipboardDataObject::SetData(FORMATETC *format, STGMEDIUM *stg,
	BOOL release)
{
	UNREFERENCED_PARAMETER(format);
	UNREFERENCED_PARAMETER(stg);
	UNREFERENCED_PARAMETER(release);

	// This class is the wrapper returned when a caller makes a simulated OleGetClipboard() call. It
	// isn't possible to assign data directly to the returned object and clients shouldn't attempt
	// to try.
	CHECK(false);

	return E_NOTIMPL;
}

IFACEMETHODIMP SimulatedClipboardDataObject::EnumFormatEtc(DWORD direction,
	IEnumFORMATETC **enumFormatEtc)
{
	return m_delegate ? m_delegate->EnumFormatEtc(direction, enumFormatEtc) : E_NOTIMPL;
}

IFACEMETHODIMP SimulatedClipboardDataObject::DAdvise(FORMATETC *format, DWORD flags,
	IAdviseSink *sink, DWORD *connection)
{
	UNREFERENCED_PARAMETER(format);
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(sink);
	UNREFERENCED_PARAMETER(connection);

	return OLE_E_ADVISENOTSUPPORTED;
}

IFACEMETHODIMP SimulatedClipboardDataObject::DUnadvise(DWORD connection)
{
	UNREFERENCED_PARAMETER(connection);

	return OLE_E_ADVISENOTSUPPORTED;
}

IFACEMETHODIMP SimulatedClipboardDataObject::EnumDAdvise(IEnumSTATDATA **enumAdvise)
{
	UNREFERENCED_PARAMETER(enumAdvise);

	return OLE_E_ADVISENOTSUPPORTED;
}

wil::com_ptr_nothrow<IDataObject> SimulatedClipboardDataObject::GetDelegate() const
{
	return m_delegate;
}

void SimulatedClipboardDataObject::SetDelegate(IDataObject *delegate)
{
	m_delegate = delegate;
}
