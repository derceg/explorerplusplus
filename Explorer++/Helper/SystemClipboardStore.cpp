// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SystemClipboardStore.h"

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

HGLOBAL SystemClipboardStore::GetData(UINT format) const
{
	return GetClipboardData(format);
}

bool SystemClipboardStore::SetData(UINT format, wil::unique_hglobal global)
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

bool SystemClipboardStore::Clear()
{
	return EmptyClipboard();
}
