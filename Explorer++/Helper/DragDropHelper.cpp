// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DragDropHelper.h"
#include "DataObjectWrapper.h"
#include "Helper.h"
#include "WinRTBaseWrapper.h"
#include <wil/com.h>

wil::unique_stg_medium GetStgMediumForGlobal(wil::unique_hglobal global)
{
	wil::unique_stg_medium storage;
	storage.tymed = TYMED_HGLOBAL;
	storage.hGlobal = global.release();
	storage.pUnkForRelease = nullptr;
	return storage;
}

HRESULT SetPreferredDropEffect(IDataObject *dataObject, DWORD effect)
{
	return SetBlobData(dataObject,
		static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT)), effect);
}

HRESULT GetPreferredDropEffect(IDataObject *dataObject, DWORD &effect)
{
	return GetBlobData(dataObject,
		static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT)), effect);
}

HRESULT SetDropDescription(IDataObject *dataObject, DROPIMAGETYPE type, const std::wstring &message,
	const std::wstring &insert)
{
	DROPDESCRIPTION dropDescription;
	dropDescription.type = type;
	StringCchCopy(dropDescription.szMessage, std::size(dropDescription.szMessage), message.c_str());
	StringCchCopy(dropDescription.szInsert, std::size(dropDescription.szInsert), insert.c_str());

	return SetBlobData(dataObject,
		static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_DROPDESCRIPTION)), dropDescription);
}

HRESULT ClearDropDescription(IDataObject *dataObject)
{
	return SetDropDescription(dataObject, DROPIMAGE_INVALID, L"", L"");
}

HRESULT CreateDataObjectForShellTransfer(const std::vector<PidlAbsolute> &items,
	IDataObject **dataObjectOut)
{
	std::vector<PCIDLIST_ABSOLUTE> rawItems;

	for (const auto &pidl : items)
	{
		rawItems.push_back(pidl.Raw());
	}

	return CreateDataObjectForShellTransfer(rawItems, dataObjectOut);
}

// Returns an IDataObject instance that can be used for clipboard operations and drag and drop.
HRESULT CreateDataObjectForShellTransfer(const std::vector<PCIDLIST_ABSOLUTE> &items,
	IDataObject **dataObjectOut)
{
	wil::com_ptr_nothrow<IShellItemArray> shellItemArray;
	RETURN_IF_FAILED(SHCreateShellItemArrayFromIDLists(static_cast<UINT>(items.size()),
		items.data(), &shellItemArray));

	wil::com_ptr_nothrow<IDataObject> shellDataObject;
	RETURN_IF_FAILED(
		shellItemArray->BindToHandler(nullptr, BHID_DataObject, IID_PPV_ARGS(&shellDataObject)));

	// Although it's possible to retrieve the IDataObjectAsyncCapability interface from the shell
	// IDataObject instance and call SetAsyncMode(), it appears that doesn't actually enable
	// asynchronous transfer. That's the reason the shell IDataObject instance is wrapped here.
	auto dataObject = winrt::make<DataObjectWrapper>(shellDataObject.get());

	// Attempting an asynchronous transfer on Windows PE will fail, so the transfer should be left
	// as synchronous (the default) in that case.
	if (!IsWindowsPE())
	{
		wil::com_ptr_nothrow<IDataObjectAsyncCapability> asyncCapability;
		RETURN_IF_FAILED(dataObject->QueryInterface(IID_PPV_ARGS(&asyncCapability)));
		RETURN_IF_FAILED(asyncCapability->SetAsyncMode(VARIANT_TRUE));
	}

	*dataObjectOut = dataObject.detach();

	return S_OK;
}

HRESULT SetBlobData(IDataObject *dataObject, CLIPFORMAT format, const void *data, size_t size)
{
	FORMATETC ftc = { format, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	return SetBlobData(dataObject, &ftc, data, size);
}

HRESULT SetBlobData(IDataObject *dataObject, FORMATETC *ftc, const void *data, size_t size)
{
	auto global = WriteDataToGlobal(data, size);

	if (!global)
	{
		return E_FAIL;
	}

	auto stg = GetStgMediumForGlobal(std::move(global));
	return MoveStorageToObject(dataObject, ftc, std::move(stg));
}

HRESULT GetBlobData(IDataObject *dataObject, CLIPFORMAT format, std::string &outputData)
{
	FORMATETC ftc = { format, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	return GetBlobData(dataObject, &ftc, outputData);
}

HRESULT GetBlobData(IDataObject *dataObject, FORMATETC *ftc, std::string &outputData)
{
	wil::unique_stg_medium stg;
	RETURN_IF_FAILED(dataObject->GetData(ftc, &stg));

	auto data = ReadBinaryDataFromGlobal(stg.hGlobal);

	if (!data)
	{
		return E_FAIL;
	}

	outputData = *data;

	return S_OK;
}
