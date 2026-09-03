// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <wincodec.h>

namespace ImageOperations
{

HRESULT ResizeBitmapToFit(IWICBitmapSource *source, UINT width, UINT height,
	WICBitmapInterpolationMode interpolationMode, wil::com_ptr_nothrow<IWICBitmapSource> &output);
HRESULT ResizeBitmap(IWICBitmapSource *source, UINT width, UINT height,
	WICBitmapInterpolationMode interpolationMode, wil::com_ptr_nothrow<IWICBitmapSource> &output);

}
