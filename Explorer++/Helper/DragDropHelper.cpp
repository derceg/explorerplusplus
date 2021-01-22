// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DragDropHelper.h"
#include "DataExchangeHelper.h"
#include "DataObjectWrapper.h"
#include <wil/com.h>

STGMEDIUM GetStgMediumForGlobal(HGLOBAL global)
{
	STGMEDIUM storage;
	storage.tymed = TYMED_HGLOBAL;
	storage.hGlobal = global;
	storage.pUnkForRelease = nullptr;
	return storage;
}

STGMEDIUM GetStgMediumForStream(IStream *stream)
{
	STGMEDIUM storage;
	storage.tymed = TYMED_ISTREAM;
	storage.pstm = stream;
	storage.pUnkForRelease = nullptr;
	return storage;
}

HRESULT SetPreferredDropEffect(IDataObject *dataObject, DWORD effect)
{
	FORMATETC ftc = { static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT)),
		nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	auto global = WriteDataToGlobal(&effect, sizeof(effect));

	if (!global)
	{
		return E_FAIL;
	}

	STGMEDIUM stg = GetStgMediumForGlobal(global.get());
	RETURN_IF_FAILED(dataObject->SetData(&ftc, &stg, TRUE));

	// The IDataObject instance has taken ownership of stg at this point, so it's responsible for
	// freeing the data.
	global.release();

	return S_OK;
}

// Returns an IDataObject instance that can be used for clipboard operations and drag and drop.
HRESULT CreateDataObjectForShellTransfer(
	const std::vector<PCIDLIST_ABSOLUTE> &items, IDataObject **dataObjectOut)
{
	wil::com_ptr_nothrow<IShellItemArray> shellItemArray;
	RETURN_IF_FAILED(SHCreateShellItemArrayFromIDLists(
		static_cast<UINT>(items.size()), items.data(), &shellItemArray));

	wil::com_ptr_nothrow<IDataObject> shellDataObject;
	RETURN_IF_FAILED(
		shellItemArray->BindToHandler(nullptr, BHID_DataObject, IID_PPV_ARGS(&shellDataObject)));

	// Although it's possible to retrieve the IDataObjectAsyncCapability interface from the shell
	// IDataObject instance and call SetAsyncMode(), it appears that doesn't actually enable
	// asynchronous transfer. That's the reason the shell IDataObject instance is wrapped here.
	wil::com_ptr_nothrow<IDataObject> dataObject = DataObjectWrapper::Create(shellDataObject.get());

	wil::com_ptr_nothrow<IDataObjectAsyncCapability> asyncCapability;
	RETURN_IF_FAILED(dataObject->QueryInterface(IID_PPV_ARGS(&asyncCapability)));
	RETURN_IF_FAILED(asyncCapability->SetAsyncMode(TRUE));

	*dataObjectOut = dataObject.detach();

	return S_OK;
}