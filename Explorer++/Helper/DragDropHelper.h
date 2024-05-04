// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DataExchangeHelper.h"
#include "PidlHelper.h"
#include <wil/result.h>
#include <ShlObj.h>
#include <shtypes.h>

wil::unique_stg_medium GetStgMediumForGlobal(wil::unique_hglobal global);
HRESULT SetPreferredDropEffect(IDataObject *dataObject, DWORD effect);
HRESULT GetPreferredDropEffect(IDataObject *dataObject, DWORD &effect);
HRESULT CreateDataObjectForShellTransfer(const std::vector<PidlAbsolute> &items,
	IDataObject **dataObjectOut);
HRESULT CreateDataObjectForShellTransfer(const std::vector<PCIDLIST_ABSOLUTE> &items,
	IDataObject **dataObjectOut);
HRESULT SetDropDescription(IDataObject *dataObject, DROPIMAGETYPE type, const std::wstring &message,
	const std::wstring &insert);
HRESULT ClearDropDescription(IDataObject *dataObject);

template <typename T>
	requires std::is_trivially_copyable_v<T> && std::is_trivially_constructible_v<T>
HRESULT SetBlobData(IDataObject *dataObject, CLIPFORMAT format, const T &data)
{
	FORMATETC ftc = { format, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	auto global = WriteDataToGlobal(&data, sizeof(data));

	if (!global)
	{
		return E_FAIL;
	}

	auto stg = GetStgMediumForGlobal(std::move(global));
	return MoveStorageToObject(dataObject, &ftc, std::move(stg));
}

template <typename T>
	requires std::is_trivially_copyable_v<T> && std::is_trivially_constructible_v<T>
HRESULT GetBlobData(IDataObject *dataObject, CLIPFORMAT format, T &outputData)
{
	FORMATETC ftc = { format, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	wil::unique_stg_medium stg;
	RETURN_IF_FAILED(dataObject->GetData(&ftc, &stg));

	auto data = ReadDataFromGlobal<T>(stg.hGlobal);

	if (!data)
	{
		return E_FAIL;
	}

	outputData = *data;

	return S_OK;
}
