// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <d2d1.h>
#include <wincodec.h>

namespace GraphicsFactories
{

IWICImagingFactory *GetWICFactory();
ID2D1Factory *GetD2DFactory();

}
