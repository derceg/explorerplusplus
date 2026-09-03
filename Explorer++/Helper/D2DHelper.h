// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <d2d1.h>
#include <wincodec.h>
#include <functional>

namespace D2DHelper
{

enum class GdiUsage
{
	WillUse,
	WontUse
};

enum class PrintWindowTarget
{
	Client,
	Full
};

HRESULT DrawToBitmap(UINT width, UINT height, GdiUsage gdiUsage,
	const std::function<HRESULT(ID2D1RenderTarget *renderTarget)> &draw,
	wil::com_ptr_nothrow<IWICBitmapSource> &output);
void DrawCenteredBitmap(ID2D1RenderTarget *renderTarget, ID2D1Bitmap *bitmap);
HRESULT DrawWindowToTarget(HWND hwnd, PrintWindowTarget printWindowTarget,
	ID2D1RenderTarget *renderTarget, POINT targetOrigin = {});
HRESULT PrintWindowToTarget(HWND hwnd, PrintWindowTarget printWindowTarget,
	ID2D1RenderTarget *renderTarget, POINT targetOrigin = {});
HRESULT DrawWithGdiInterop(ID2D1RenderTarget *renderTarget, D2D1_DC_INITIALIZE_MODE initializeMode,
	const std::function<HRESULT(HDC hdc)> &draw);
D2D1::ColorF ColorFromColorRef(COLORREF color);

}
