// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ResourceLoader.h"

class ResourceLoaderFake : public ResourceLoader
{
public:
	std::wstring LoadString(UINT stringId) const override;
	std::optional<std::wstring> MaybeLoadString(UINT stringId) const override;

	wil::unique_hbitmap LoadBitmapFromPNGForDpi(Icon icon, int iconWidth, int iconHeight,
		int dpi) const override;
	wil::unique_hbitmap LoadBitmapFromPNGAndScale(Icon icon, int iconWidth,
		int iconHeight) const override;
	wil::unique_hicon LoadIconFromPNGForDpi(Icon icon, int iconWidth, int iconHeight,
		int dpi) const override;
	wil::unique_hicon LoadIconFromPNGAndScale(Icon icon, int iconWidth,
		int iconHeight) const override;
};
