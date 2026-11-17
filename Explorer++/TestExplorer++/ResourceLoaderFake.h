// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ResourceLoader.h"

class ResourceLoaderFake : public ResourceLoader
{
public:
	std::wstring LoadString(UINT stringId) const override;

	wil::com_ptr_nothrow<IWICBitmapSource> LoadBitmapForDpi(Icon icon, int iconWidth,
		int iconHeight, int dpi) const override;
	wil::com_ptr_nothrow<IWICBitmapSource> LoadBitmap(Icon icon, int iconWidth,
		int iconHeight) const override;

	INT_PTR CreateModalDialog(UINT dialogId, HWND parent, DialogProc dialogProc) const override;
	HWND CreateModelessDialog(UINT dialogId, HWND parent, DialogProc dialogProc) const override;
};
