// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SimulatedClipboardStore.h"
#include "SimulatedClipboardDataObject.h"
#include "../Helper/DataExchangeHelper.h"
#include "../Helper/DataObjectImpl.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/WilExtraTypes.h"

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
	wil::com_ptr_nothrow<IEnumFORMATETC> enumFormats;
	HRESULT hr = m_dataObject->EnumFormatEtc(DATADIR_GET, &enumFormats);

	if (FAILED(hr))
	{
		return false;
	}

	unique_formatetc formatEtc;

	while (enumFormats->Next(1, &formatEtc, nullptr) == S_OK)
	{
		if (formatEtc.cfFormat == format)
		{
			return true;
		}
	}

	return false;
}

wil::unique_hglobal SimulatedClipboardStore::GetGlobalData(UINT format) const
{
	FORMATETC formatEtc = GetGlobalFormatEtc(format);
	wil::unique_stg_medium stgMedium;
	HRESULT hr = m_dataObject->GetData(&formatEtc, &stgMedium);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return CloneGlobal(stgMedium.hGlobal);
}

bool SimulatedClipboardStore::SetGlobalData(UINT format, wil::unique_hglobal global)
{
	// It's only possible to add data to the underlying delegate if that delegate was created by
	// this class.
	//
	// If the delegate is external (supplied via a call to SetDataObject()), it's not possible to
	// add additional data using this method and clients shouldn't attempt to do so.
	CHECK(m_delegateType == DelegateType::Internal);

	FORMATETC formatEtc = GetGlobalFormatEtc(format);
	auto stg = GetStgMediumForGlobal(std::move(global));

	auto delegate = m_dataObject->GetDelegate();
	return (MoveStorageToObject(delegate.get(), &formatEtc, std::move(stg)) == S_OK);
}

wil::unique_hbitmap SimulatedClipboardStore::GetGdiBitmap() const
{
	FORMATETC formatEtc = GetGdiBitmapFormatEtc();
	wil::unique_stg_medium stgMedium;
	HRESULT hr = m_dataObject->GetData(&formatEtc, &stgMedium);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return wil::unique_hbitmap(static_cast<HBITMAP>(
		CopyImage(stgMedium.hBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION)));
}

bool SimulatedClipboardStore::SetGdiBitmap(wil::unique_hbitmap bitmap)
{
	CHECK(m_delegateType == DelegateType::Internal);

	FORMATETC formatEtc = GetGdiBitmapFormatEtc();
	auto stg = GetStgMediumForGdiBitmap(std::move(bitmap));

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

FORMATETC SimulatedClipboardStore::GetGlobalFormatEtc(UINT format)
{
	return { static_cast<CLIPFORMAT>(format), nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
}

FORMATETC SimulatedClipboardStore::GetGdiBitmapFormatEtc()
{
	return { CF_BITMAP, nullptr, DVASPECT_CONTENT, -1, TYMED_GDI };
}
