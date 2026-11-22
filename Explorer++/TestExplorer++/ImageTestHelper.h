// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <wincodec.h>

wil::com_ptr_nothrow<IWICBitmapSource> BuildTestBitmap(UINT width, UINT height);
bool AreBitmapsEquivalent(IWICBitmapSource *bitmap1, IWICBitmapSource *bitmap2);
