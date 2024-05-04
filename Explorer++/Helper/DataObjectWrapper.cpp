// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DataObjectWrapper.h"

DataObjectWrapper::DataObjectWrapper(IDataObject *dataObject) : m_dataObject(dataObject)
{
}

// IDataObject
IFACEMETHODIMP DataObjectWrapper::GetData(FORMATETC *formatEtc, STGMEDIUM *medium)
{
	return m_dataObject->GetData(formatEtc, medium);
}

IFACEMETHODIMP DataObjectWrapper::GetDataHere(FORMATETC *formatEtc, STGMEDIUM *medium)
{
	return m_dataObject->GetDataHere(formatEtc, medium);
}

IFACEMETHODIMP DataObjectWrapper::QueryGetData(FORMATETC *formatEtc)
{
	return m_dataObject->QueryGetData(formatEtc);
}

IFACEMETHODIMP DataObjectWrapper::GetCanonicalFormatEtc(FORMATETC *formatEtc,
	FORMATETC *formatEtcResult)
{
	return m_dataObject->GetCanonicalFormatEtc(formatEtc, formatEtcResult);
}

IFACEMETHODIMP DataObjectWrapper::SetData(FORMATETC *formatEtc, STGMEDIUM *medium,
	BOOL shouldRelease)
{
	return m_dataObject->SetData(formatEtc, medium, shouldRelease);
}

IFACEMETHODIMP DataObjectWrapper::EnumFormatEtc(DWORD direction, IEnumFORMATETC **enumerator)
{
	return m_dataObject->EnumFormatEtc(direction, enumerator);
}

IFACEMETHODIMP DataObjectWrapper::DAdvise(FORMATETC *formatEtc, DWORD advf, IAdviseSink *sink,
	DWORD *connection)
{
	return m_dataObject->DAdvise(formatEtc, advf, sink, connection);
}

IFACEMETHODIMP DataObjectWrapper::DUnadvise(DWORD connection)
{
	return m_dataObject->DUnadvise(connection);
}

IFACEMETHODIMP DataObjectWrapper::EnumDAdvise(IEnumSTATDATA **enumerator)
{
	return m_dataObject->EnumDAdvise(enumerator);
}

// IDataObjectAsyncCapability
IFACEMETHODIMP DataObjectWrapper::GetAsyncMode(BOOL *isOpAsync)
{
	*isOpAsync = m_isOpAsync ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

IFACEMETHODIMP DataObjectWrapper::SetAsyncMode(BOOL doOpAsync)
{
	m_isOpAsync = !!doOpAsync;

	return S_OK;
}

IFACEMETHODIMP DataObjectWrapper::InOperation(BOOL *inAsyncOp)
{
	*inAsyncOp = m_inOperation ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

IFACEMETHODIMP DataObjectWrapper::StartOperation(IBindCtx *reserved)
{
	UNREFERENCED_PARAMETER(reserved);

	m_inOperation = true;

	return S_OK;
}

IFACEMETHODIMP DataObjectWrapper::EndOperation(HRESULT result, IBindCtx *reserved, DWORD effects)
{
	UNREFERENCED_PARAMETER(result);
	UNREFERENCED_PARAMETER(reserved);
	UNREFERENCED_PARAMETER(effects);

	m_inOperation = false;
	return S_OK;
}
