// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DataExchangeHelper.h"
#include "PidlHelper.h"
#include "UniqueVariableSizeStruct.h"
#include <wil/result.h>
#include <ShlObj.h>
#include <shtypes.h>
#include <optional>

wil::unique_stg_medium GetStgMediumForGlobal(wil::unique_hglobal global);
HRESULT SetPreferredDropEffect(IDataObject *dataObject, DWORD effect);
HRESULT GetPreferredDropEffect(IDataObject *dataObject, DWORD &effect);
HRESULT SetDropDescription(IDataObject *dataObject, DROPIMAGETYPE type, const std::wstring &message,
	const std::wstring &insert);
HRESULT ClearDropDescription(IDataObject *dataObject);
HRESULT StartDragForShellItems(const std::vector<PCIDLIST_ABSOLUTE> &items,
	std::optional<DWORD> preferredDropEffect = std::nullopt);
HRESULT CreateDataObjectForShellTransfer(const std::vector<PidlAbsolute> &items,
	IDataObject **dataObjectOut);
HRESULT CreateDataObjectForShellTransfer(const std::vector<PCIDLIST_ABSOLUTE> &items,
	IDataObject **dataObjectOut);
HRESULT GetTextFromDataObject(IDataObject *dataObject, std::wstring &outputText);
HRESULT SetTextOnDataObject(IDataObject *dataObject, const std::wstring &text);
HRESULT SetBlobData(IDataObject *dataObject, CLIPFORMAT format, const void *data, size_t size);
HRESULT SetBlobData(IDataObject *dataObject, FORMATETC *ftc, const void *data, size_t size);
HRESULT GetBlobData(IDataObject *dataObject, CLIPFORMAT format, std::string &outputData);
HRESULT GetBlobData(IDataObject *dataObject, FORMATETC *ftc, std::string &outputData);

template <typename T>
	requires std::is_trivially_copyable_v<T> && std::is_trivially_constructible_v<T>
HRESULT SetBlobData(IDataObject *dataObject, CLIPFORMAT format, const T &data)
{
	return SetBlobData(dataObject, format, &data, sizeof(data));
}

template <typename T>
	requires std::is_trivially_copyable_v<T> && std::is_trivially_constructible_v<T>
HRESULT GetBlobData(IDataObject *dataObject, CLIPFORMAT format, T &outputData)
{
	std::string binaryData;
	RETURN_IF_FAILED(GetBlobData(dataObject, format, binaryData));

	// The size of the data that's retrieved is determined by GlobalSize(). As indicated by the
	// documentation for that function, the returned size can be larger than the size requested when
	// the memory was allocated. If the size is smaller than the size of the target type, however,
	// something has gone wrong and it doesn't make sense to try and use the data.
	if (binaryData.size() < sizeof(T))
	{
		return E_FAIL;
	}

	std::memcpy(&outputData, binaryData.data(), sizeof(T));

	return S_OK;
}

template <typename T>
HRESULT GetBlobData(IDataObject *dataObject, CLIPFORMAT format,
	UniqueVariableSizeStruct<T> &outputData)
{
	std::string binaryData;
	RETURN_IF_FAILED(GetBlobData(dataObject, format, binaryData));

	if (binaryData.size() < sizeof(T))
	{
		return E_FAIL;
	}

	outputData = MakeUniqueVariableSizeStruct<T>(binaryData.size());
	std::memcpy(outputData.get(), binaryData.data(), binaryData.size());

	return S_OK;
}
