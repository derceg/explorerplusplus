// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <wil/resource.h>
#include <wincodec.h>

namespace ImageInterop
{

HRESULT CreateWICBitmapFromHICON(HICON source, wil::com_ptr_nothrow<IWICBitmapSource> &output);
HRESULT CreateHBITMAPFromWICBitmap(IWICBitmapSource *source, wil::unique_hbitmap &output);

}
