// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IconResourceLoader.h"
#include "DarkModeHelper.h"
#include "IconMappings.h"
#include "../Helper/ImageHelper.h"

IconResourceLoader::IconResourceLoader(IconSet iconSet, const DarkModeHelper *darkModeHelper) :
	m_iconSet(iconSet),
	m_darkModeHelper(darkModeHelper)
{
}

wil::unique_hbitmap IconResourceLoader::LoadBitmapFromPNGForDpi(Icon icon, int iconWidth,
	int iconHeight, int dpi) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGForDpi(icon, iconWidth, iconHeight, dpi);
	return ImageHelper::GdiplusBitmapToBitmap(gdiplusBitmap.get());
}

wil::unique_hbitmap IconResourceLoader::LoadBitmapFromPNGAndScale(Icon icon, int iconWidth,
	int iconHeight) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGAndScalePlusInvert(icon, iconWidth, iconHeight);
	return ImageHelper::GdiplusBitmapToBitmap(gdiplusBitmap.get());
}

wil::unique_hicon IconResourceLoader::LoadIconFromPNGForDpi(Icon icon, int iconWidth,
	int iconHeight, int dpi) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGForDpi(icon, iconWidth, iconHeight, dpi);
	return ImageHelper::GdiplusBitmapToIcon(gdiplusBitmap.get());
}

wil::unique_hicon IconResourceLoader::LoadIconFromPNGAndScale(Icon icon, int iconWidth,
	int iconHeight) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGAndScalePlusInvert(icon, iconWidth, iconHeight);
	return ImageHelper::GdiplusBitmapToIcon(gdiplusBitmap.get());
}

std::unique_ptr<Gdiplus::Bitmap> IconResourceLoader::LoadGdiplusBitmapFromPNGForDpi(Icon icon,
	int iconWidth, int iconHeight, int dpi) const
{
	int scaledIconWidth = MulDiv(iconWidth, dpi, USER_DEFAULT_SCREEN_DPI);
	int scaledIconHeight = MulDiv(iconHeight, dpi, USER_DEFAULT_SCREEN_DPI);
	return LoadGdiplusBitmapFromPNGAndScalePlusInvert(icon, scaledIconWidth, scaledIconHeight);
}

// Loads and scales a PNG and then inverts the colors, when required by the current color mode.
std::unique_ptr<Gdiplus::Bitmap> IconResourceLoader::LoadGdiplusBitmapFromPNGAndScalePlusInvert(
	Icon icon, int iconWidth, int iconHeight) const
{
	auto bitmap = LoadGdiplusBitmapFromPNGAndScale(icon, iconWidth, iconHeight);

	if (m_iconSet == +IconSet::Color || !m_darkModeHelper->IsDarkModeEnabled())
	{
		return bitmap;
	}

	auto invertedBitmap = std::make_unique<Gdiplus::Bitmap>(iconWidth, iconHeight);
	invertedBitmap->SetResolution(bitmap->GetHorizontalResolution(),
		bitmap->GetVerticalResolution());

	Gdiplus::Graphics graphics(invertedBitmap.get());

	// This matrix will result in the RGB components all being inverted, while the alpha component
	// will stay as-is. See the documentation on ColorMatrix for information on how this structure
	// is laid out.
	// clang-format off
	Gdiplus::ColorMatrix colorMatrix = {
		-1, 0, 0, 0, 0,
		0, -1, 0, 0, 0,
		0, 0, -1, 0, 0,
		0, 0, 0, 1, 0,
		1, 1, 1, 0, 1
	};
	// clang-format on

	Gdiplus::ImageAttributes attributes;
	attributes.SetColorMatrix(&colorMatrix);

	graphics.DrawImage(bitmap.get(), Gdiplus::Rect(0, 0, iconWidth, iconHeight), 0, 0, iconWidth,
		iconHeight, Gdiplus::UnitPixel, &attributes);

	return invertedBitmap;
}

// This function is based on the steps performed by
// https://docs.microsoft.com/en-us/windows/win32/api/commctrl/nf-commctrl-loadiconmetric when
// loading an icon (see the remarks section on that page for details).
std::unique_ptr<Gdiplus::Bitmap> IconResourceLoader::LoadGdiplusBitmapFromPNGAndScale(Icon icon,
	int iconWidth, int iconHeight) const
{
	const IconMapping *mapping = nullptr;

	switch (m_iconSet)
	{
	case IconSet::Color:
		mapping = &ICON_RESOURCE_MAPPINGS_COLOR;
		break;

	case IconSet::FluentUi:
		mapping = &ICON_RESOURCE_MAPPINGS_FLUENT_UI;
		break;

	case IconSet::Windows10:
		mapping = &ICON_RESOURCE_MAPPINGS_WINDOWS_10;
		break;
	}

	const auto &iconSizeMappins = mapping->at(icon);

	auto match = std::find_if(iconSizeMappins.begin(), iconSizeMappins.end(),
		[iconWidth, iconHeight](auto entry)
		{ return iconWidth <= entry.first && iconHeight <= entry.first; });

	if (match == iconSizeMappins.end())
	{
		match = std::prev(iconSizeMappins.end());
	}

	auto bitmap = ImageHelper::LoadGdiplusBitmapFromPNG(GetModuleHandle(nullptr), match->second);
	assert(bitmap);

	// If the icon size matches exactly, it doesn't need to be scaled, so can be
	// returned immediately.
	if (match->first == iconWidth && match->first == iconHeight)
	{
		return bitmap;
	}

	auto scaledBitmap = std::make_unique<Gdiplus::Bitmap>(iconWidth, iconHeight);
	scaledBitmap->SetResolution(bitmap->GetHorizontalResolution(), bitmap->GetVerticalResolution());

	Gdiplus::Graphics graphics(scaledBitmap.get());

	float scalingFactorX = static_cast<float>(iconWidth) / static_cast<float>(match->first);
	float scalingFactorY = static_cast<float>(iconHeight) / static_cast<float>(match->first);
	graphics.ScaleTransform(scalingFactorX, scalingFactorY);
	graphics.DrawImage(bitmap.get(), 0, 0);

	return scaledBitmap;
}
