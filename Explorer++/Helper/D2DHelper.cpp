// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "D2DHelper.h"
#include "GraphicsFactories.h"
#include "WindowHelper.h"
#include <wil/resource.h>
#include <wil/result_macros.h>
#include <wingdi.h>

namespace
{

class ScopedDCState
{
public:
	ScopedDCState(HDC hdc) : m_hdc(hdc), m_savedDc(SaveDC(hdc))
	{
	}

	bool IsValid() const
	{
		return m_savedDc != 0;
	}

	~ScopedDCState()
	{
		if (m_savedDc != 0)
		{
			auto res = RestoreDC(m_hdc, m_savedDc);
			DCHECK(res);
		}
	}

private:
	const HDC m_hdc;
	const int m_savedDc;
};

class ScopedShowWindowForPrint
{
public:
	ScopedShowWindowForPrint(HWND hwnd) : m_hwnd(hwnd), m_visible(IsWindowVisible(hwnd))
	{
		if (!m_visible)
		{
			ShowWindow(m_hwnd, SW_SHOWNA);
		}
	}

	~ScopedShowWindowForPrint()
	{
		if (!m_visible)
		{
			ShowWindow(m_hwnd, SW_HIDE);
		}
	}

private:
	const HWND m_hwnd;
	const bool m_visible;
};

}

namespace D2DHelper
{

HRESULT DrawToBitmap(UINT width, UINT height, GdiUsage gdiUsage,
	const std::function<HRESULT(ID2D1RenderTarget *renderTarget)> &draw,
	wil::com_ptr_nothrow<IWICBitmapSource> &output)
{
	WICPixelFormatGUID pixelFormat;
	D2D1_ALPHA_MODE alphaMode;
	D2D1_RENDER_TARGET_USAGE targetUsage;

	if (gdiUsage == GdiUsage::WillUse)
	{
		// GDI typically doesn't consider alpha at all and pixels it writes won't have any alpha
		// information set. So, using a pixel format that doesn't contain a logical alpha channel
		// makes the most sense.
		pixelFormat = GUID_WICPixelFormat32bppBGR;
		alphaMode = D2D1_ALPHA_MODE_IGNORE;
		targetUsage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
	}
	else
	{
		pixelFormat = GUID_WICPixelFormat32bppPBGRA;
		alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
		targetUsage = D2D1_RENDER_TARGET_USAGE_NONE;
	}

	wil::com_ptr_nothrow<IWICBitmap> bitmap;
	auto *imagingFactory = GraphicsFactories::GetWICFactory();
	RETURN_IF_FAILED(
		imagingFactory->CreateBitmap(width, height, pixelFormat, WICBitmapCacheOnLoad, &bitmap));

	auto renderTargetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, alphaMode), 0, 0, targetUsage);

	wil::com_ptr_nothrow<ID2D1RenderTarget> renderTarget;
	auto *d2dFactory = GraphicsFactories::GetD2DFactory();
	RETURN_IF_FAILED(d2dFactory->CreateWicBitmapRenderTarget(bitmap.get(), renderTargetProperties,
		&renderTarget));

	renderTarget->BeginDraw();
	HRESULT drawResult = draw(renderTarget.get());
	HRESULT endDrawResult = renderTarget->EndDraw();

	RETURN_IF_FAILED(drawResult);
	RETURN_IF_FAILED(endDrawResult);

	output = bitmap;

	return S_OK;
}

void DrawCenteredBitmap(ID2D1RenderTarget *renderTarget, ID2D1Bitmap *bitmap)
{
	auto targetSize = renderTarget->GetPixelSize();
	auto bitmapSize = bitmap->GetSize();
	float x = (targetSize.width - bitmapSize.width) / 2.0f;
	float y = (targetSize.height - bitmapSize.height) / 2.0f;
	renderTarget->DrawBitmap(bitmap,
		D2D1::RectF(x, y, x + bitmapSize.width, y + bitmapSize.height));
}

HRESULT DrawWindowToTarget(HWND hwnd, PrintWindowTarget printWindowTarget,
	ID2D1RenderTarget *renderTarget, POINT targetOrigin)
{
	return DrawWithGdiInterop(renderTarget, D2D1_DC_INITIALIZE_MODE_COPY,
		[hwnd, printWindowTarget, targetOrigin](HDC hdc)
		{
			ScopedDCState dcState(hdc);
			RETURN_HR_IF(E_FAIL, !dcState.IsValid());

			RETURN_IF_WIN32_BOOL_FALSE(
				SetViewportOrgEx(hdc, targetOrigin.x, targetOrigin.y, nullptr));

			wil::unique_hdc_window hdcSrc;
			RECT rect;

			if (printWindowTarget == PrintWindowTarget::Full)
			{
				hdcSrc = wil::GetWindowDC(hwnd);
				RETURN_IF_WIN32_BOOL_FALSE(GetWindowRect(hwnd, &rect));
			}
			else
			{
				hdcSrc = wil::GetDC(hwnd);
				RETURN_IF_WIN32_BOOL_FALSE(GetClientRect(hwnd, &rect));
			}

			RETURN_HR_IF_NULL(E_FAIL, hdcSrc);

			RETURN_IF_WIN32_BOOL_FALSE(BitBlt(hdc, 0, 0, GetRectWidth(&rect), GetRectHeight(&rect),
				hdcSrc.get(), 0, 0, SRCCOPY));

			return S_OK;
		});
}

HRESULT PrintWindowToTarget(HWND hwnd, PrintWindowTarget printWindowTarget,
	ID2D1RenderTarget *renderTarget, POINT targetOrigin)
{
	// The window needs to be at least temporarily visible for the PrintWindow() call to work. Even
	// when using something like WM_PRINT without the PRF_CHECKVISIBLE option, it's still a good
	// idea to show the window first, because the rendering that takes place can be affected by
	// visibility.
	ScopedShowWindowForPrint scopedShowWindow(hwnd);

	return DrawWithGdiInterop(renderTarget, D2D1_DC_INITIALIZE_MODE_COPY,
		[hwnd, printWindowTarget, targetOrigin](HDC hdc)
		{
			ScopedDCState dcState(hdc);
			RETURN_HR_IF(E_FAIL, !dcState.IsValid());

			RETURN_IF_WIN32_BOOL_FALSE(
				SetViewportOrgEx(hdc, targetOrigin.x, targetOrigin.y, nullptr));
			RETURN_IF_WIN32_BOOL_FALSE(PrintWindow(hwnd, hdc,
				printWindowTarget == PrintWindowTarget::Full ? 0 : PW_CLIENTONLY));

			return S_OK;
		});
}

HRESULT DrawWithGdiInterop(ID2D1RenderTarget *renderTarget, D2D1_DC_INITIALIZE_MODE initializeMode,
	const std::function<HRESULT(HDC hdc)> &draw)
{
	wil::com_ptr_nothrow<ID2D1GdiInteropRenderTarget> gdiInterop;
	RETURN_IF_FAILED(renderTarget->QueryInterface(&gdiInterop));

	HDC hdc;
	RETURN_IF_FAILED(gdiInterop->GetDC(initializeMode, &hdc));
	HRESULT drawResult = draw(hdc);
	HRESULT releaseResult = gdiInterop->ReleaseDC(nullptr);

	RETURN_IF_FAILED(drawResult);
	RETURN_IF_FAILED(releaseResult);

	return S_OK;
}

D2D1::ColorF ColorFromColorRef(COLORREF color)
{
	return { GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f };
}

}
