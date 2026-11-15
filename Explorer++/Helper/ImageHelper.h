// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <CommCtrl.h>
#include <commoncontrols.h>
#include <wincodec.h>

namespace ImageHelper
{

HRESULT LoadBitmapFromPNG(HINSTANCE resourceInstance, UINT resourceId,
	wil::com_ptr_nothrow<IWICBitmapSource> &output);

HRESULT InvertBitmap(IWICBitmapSource *source, wil::com_ptr_nothrow<IWICBitmapSource> &output);

int AddBitmapToImageList(HIMAGELIST imageList, IWICBitmapSource *bitmap);
HRESULT CreateBitmapFromImageListIcon(IImageList *imageList, int iconIndex,
	wil::com_ptr_nothrow<IWICBitmapSource> &output);
int CopyImageListIcon(HIMAGELIST destination, HIMAGELIST source, int sourceIconIndex);

}
