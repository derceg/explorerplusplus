// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ClipboardStore.h"

// This class simply forwards each call to the system clipboard.
class SystemClipboardStore : public ClipboardStore
{
public:
	bool Open() override;
	bool Close() override;

	bool IsDataAvailable(UINT format) const override;

	wil::unique_hglobal GetGlobalData(UINT format) const override;
	bool SetGlobalData(UINT format, wil::unique_hglobal global) override;

	wil::unique_hbitmap GetGdiBitmap() const override;
	bool SetGdiBitmap(wil::unique_hbitmap bitmap) override;

	wil::com_ptr_nothrow<IDataObject> GetDataObject() const override;
	bool SetDataObject(IDataObject *dataObject) override;
	bool IsDataObjectCurrent(IDataObject *dataObject) const override;
	bool FlushDataObject() override;

	bool Clear() override;
};
