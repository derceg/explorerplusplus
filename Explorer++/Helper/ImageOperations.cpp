// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ImageOperations.h"
#include "GraphicsFactories.h"

namespace ImageOperations
{

HRESULT ResizeBitmapToFit(IWICBitmapSource *source, UINT width, UINT height,
	WICBitmapInterpolationMode interpolationMode, wil::com_ptr_nothrow<IWICBitmapSource> &output)
{
	UINT sourceWidth;
	UINT sourceHeight;
	RETURN_IF_FAILED(source->GetSize(&sourceWidth, &sourceHeight));

	float scale = std::min({ 1.0f, static_cast<float>(width) / sourceWidth,
		static_cast<float>(height) / sourceHeight });
	UINT finalWidth = static_cast<UINT>(std::ceil(sourceWidth * scale));
	UINT finalHeight = static_cast<UINT>(std::ceil(sourceHeight * scale));

	RETURN_IF_FAILED(ResizeBitmap(source, finalWidth, finalHeight, interpolationMode, output));

	return S_OK;
}

HRESULT ResizeBitmap(IWICBitmapSource *source, UINT width, UINT height,
	WICBitmapInterpolationMode interpolationMode, wil::com_ptr_nothrow<IWICBitmapSource> &output)
{
	wil::com_ptr_nothrow<IWICBitmapScaler> scaler;
	auto *imagingFactory = GraphicsFactories::GetWICFactory();
	RETURN_IF_FAILED(imagingFactory->CreateBitmapScaler(&scaler));
	RETURN_IF_FAILED(scaler->Initialize(source, width, height, interpolationMode));
	output = scaler;
	return S_OK;
}

}
