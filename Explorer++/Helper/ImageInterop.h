// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <wil/resource.h>
#include <wincodec.h>

namespace ImageInterop
{

enum class AlphaMode
{
	Straight,
	Premultiplied
};

HRESULT CreateWICBitmapFromHICON(HICON source, wil::com_ptr_nothrow<IWICBitmapSource> &output);
HRESULT CreateHICONFromWICBitmap(IWICBitmapSource *source, wil::unique_hicon &output);
HRESULT CreateHBITMAPFromWICBitmap(IWICBitmapSource *source, AlphaMode alphaMode,
	wil::unique_hbitmap &output);

}
