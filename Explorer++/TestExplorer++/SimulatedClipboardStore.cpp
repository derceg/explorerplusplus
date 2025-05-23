// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedClipboardStore.h"
#include "SimulatedClipboardDataObject.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/DragDropHelper.h"

SimulatedClipboardStore::SimulatedClipboardStore() :
	m_dataObject(winrt::make_self<SimulatedClipboardDataObject>())
{
	ResetDelegate();
}

SimulatedClipboardStore::~SimulatedClipboardStore() = default;

bool SimulatedClipboardStore::Open()
{
	// This is a no-op, since this class doesn't model clipboard ownership at all.
	return true;
}

bool SimulatedClipboardStore::Close()
{
	return true;
}

bool SimulatedClipboardStore::IsDataAvailable(UINT format) const
{
	FORMATETC formatEtc = GetFormatEtc(format);
	return (m_dataObject->QueryGetData(&formatEtc) == S_OK);
}

wil::unique_hglobal SimulatedClipboardStore::GetData(UINT format) const
{
	FORMATETC formatEtc = GetFormatEtc(format);
	wil::unique_stg_medium stgMedium;
	HRESULT hr = m_dataObject->GetData(&formatEtc, &stgMedium);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return CloneGlobal(stgMedium.hGlobal);
}

bool SimulatedClipboardStore::SetData(UINT format, wil::unique_hglobal global)
{
	// It's only possible to add data to the underlying delegate if that delegate was created by
	// this class.
	//
	// If the delegate is external (supplied via a call to SetDataObject()), it's not possible to
	// add additional data using this method and clients shouldn't attempt to do so.
	CHECK(m_delegateType == DelegateType::Internal);

	FORMATETC formatEtc = GetFormatEtc(format);
	auto stg = GetStgMediumForGlobal(std::move(global));

	auto delegate = m_dataObject->GetDelegate();
	return (MoveStorageToObject(delegate.get(), &formatEtc, std::move(stg)) == S_OK);
}

wil::com_ptr_nothrow<IDataObject> SimulatedClipboardStore::GetDataObject() const
{
	return wil::com_ptr_nothrow<IDataObject>(m_dataObject.get());
}

bool SimulatedClipboardStore::SetDataObject(IDataObject *dataObject)
{
	SetExternalDelegate(dataObject);
	return true;
}

bool SimulatedClipboardStore::IsDataObjectCurrent(IDataObject *dataObject) const
{
	if (!dataObject || m_delegateType != DelegateType::External)
	{
		return false;
	}

	return dataObject == m_dataObject->GetDelegate();
}

bool SimulatedClipboardStore::FlushDataObject()
{
	// Typically, it's necessary to flush the clipboard before exiting the application, to ensure
	// the data will remain on the clipboard. However, there's no need to do anything here, given
	// that this is an in-process store.
	return true;
}

bool SimulatedClipboardStore::Clear()
{
	ResetDelegate();
	return true;
}

void SimulatedClipboardStore::SetExternalDelegate(IDataObject *externalDelegate)
{
	m_dataObject->SetDelegate(externalDelegate);
	m_delegateType = DelegateType::External;
}

// Resets the current delegate back to a newly created internal delegate. As the delegate contains
// all clipboard data, this effectively clears the clipboard.
void SimulatedClipboardStore::ResetDelegate()
{
	auto internalDelegate = winrt::make<DataObjectImpl>();
	m_dataObject->SetDelegate(internalDelegate.get());
	m_delegateType = DelegateType::Internal;
}

FORMATETC SimulatedClipboardStore::GetFormatEtc(UINT format)
{
	return { static_cast<CLIPFORMAT>(format), nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
}
