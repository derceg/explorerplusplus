// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <gdiplus.h>
#include <memory>

void BuildTestBitmap(int width, int height, std::unique_ptr<Gdiplus::Bitmap> &bitmap);
bool AreBitmapsEquivalent(Gdiplus::Bitmap *bitmap1, Gdiplus::Bitmap *bitmap2);
