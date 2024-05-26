// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <gdiplus.h>
#include <optional>
#include <string>

namespace GdiplusHelper
{

std::optional<CLSID> GetEncoderClsid(const std::wstring &format);
std::unique_ptr<Gdiplus::Bitmap> DeepCopyBitmap(Gdiplus::Bitmap *bitmap);

}
