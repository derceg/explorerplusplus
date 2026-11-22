// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SystemClipboardStore.h"
#include "DataExchangeHelper.h"

bool SystemClipboardStore::Open()
{
	return OpenClipboard(nullptr);
}

bool SystemClipboardStore::Close()
{
	return CloseClipboard();
}

bool SystemClipboardStore::IsDataAvailable(UINT format) const
{
	return IsClipboardFormatAvailable(format);
}

wil::unique_hglobal SystemClipboardStore::GetGlobalData(UINT format) const
{
	auto data = GetClipboardData(format);

	if (!data)
	{
		return nullptr;
	}

	return CloneGlobal(data);
}

bool SystemClipboardStore::SetGlobalData(UINT format, wil::unique_hglobal global)
{
	HANDLE clipboardData = SetClipboardData(format, global.get());

	if (!clipboardData)
	{
		return false;
	}

	// SetClipboardData() takes ownership of the data passed to it. Therefore, it's important that
	// the ownership of the data is relinquished here if the call to SetClipboardData() succeeded.
	global.release();

	return true;
}

wil::unique_hbitmap SystemClipboardStore::GetGdiBitmap() const
{
	auto gdiBitmap = GetClipboardData(CF_BITMAP);

	if (!gdiBitmap)
	{
		return nullptr;
	}

	return wil::unique_hbitmap(
		static_cast<HBITMAP>(CopyImage(gdiBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION)));
}

bool SystemClipboardStore::SetGdiBitmap(wil::unique_hbitmap bitmap)
{
	HANDLE clipboardData = SetClipboardData(CF_BITMAP, bitmap.get());

	if (!clipboardData)
	{
		return false;
	}

	bitmap.release();

	return true;
}

wil::com_ptr_nothrow<IDataObject> SystemClipboardStore::GetDataObject() const
{
	wil::com_ptr_nothrow<IDataObject> dataObject;
	HRESULT hr = OleGetClipboard(&dataObject);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return dataObject;
}

bool SystemClipboardStore::SetDataObject(IDataObject *dataObject)
{
	return (OleSetClipboard(dataObject) == S_OK);
}

bool SystemClipboardStore::IsDataObjectCurrent(IDataObject *dataObject) const
{
	return (OleIsCurrentClipboard(dataObject) == S_OK);
}

bool SystemClipboardStore::FlushDataObject()
{
	return (OleFlushClipboard() == S_OK);
}

bool SystemClipboardStore::Clear()
{
	return EmptyClipboard();
}
