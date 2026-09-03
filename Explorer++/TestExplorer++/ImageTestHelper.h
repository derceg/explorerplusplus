// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <wil/resource.h>
#include <gdiplus.h>
#include <wincodec.h>
#include <memory>

wil::com_ptr_nothrow<IWICBitmapSource> BuildTestWICBitmap(UINT width, UINT height);
void BuildTestBitmap(int width, int height, wil::unique_hbitmap &bitmap);
void BuildTestGdiplusBitmap(int width, int height, std::unique_ptr<Gdiplus::Bitmap> &bitmap);
bool AreGdiplusBitmapsEquivalent(Gdiplus::Bitmap *bitmap1, Gdiplus::Bitmap *bitmap2);
