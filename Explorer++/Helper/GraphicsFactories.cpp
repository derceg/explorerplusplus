// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "GraphicsFactories.h"
#include <wil/com.h>

namespace GraphicsFactories
{

IWICImagingFactory *GetWICFactory()
{
	// This instance lives for the lifetime of the application and is deliberately leaked.
	static IWICImagingFactory *factory = []
	{
		auto imagingFactory =
			wil::CoCreateInstanceNoThrow<IWICImagingFactory>(CLSID_WICImagingFactory);
		CHECK(imagingFactory);
		return imagingFactory.detach();
	}();

	return factory;
}

ID2D1Factory *GetD2DFactory()
{
	static ID2D1Factory *factory = []
	{
		wil::com_ptr_nothrow<ID2D1Factory> d2dFactory;
		FAIL_FAST_IF_FAILED(
			D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&d2dFactory)));
		return d2dFactory.detach();
	}();

	return factory;
}

}
