// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IconResourceLoader.h"
#include "IconMappingsColor.h"
#include "IconMappingsWindows10.h"
#include "../Helper/ImageHelper.h"

IconResourceLoader::IconResourceLoader(IconTheme iconTheme) :
	m_iconTheme(iconTheme)
{
	assert(ICON_RESOURCE_MAPPINGS_COLOR.size() == ICON_RESOURCE_MAPPINGS_WINDOWS_10.size());
}

wil::unique_hbitmap IconResourceLoader::LoadBitmapFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGForDpi(icon, iconWidth, iconHeight, dpi);
	return RetrieveBitmapFromGdiplusBitmap(gdiplusBitmap.get());
}

wil::unique_hbitmap IconResourceLoader::LoadBitmapFromPNGAndScale(Icon icon, int iconWidth, int iconHeight) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGAndScale(icon, iconWidth, iconHeight);
	return RetrieveBitmapFromGdiplusBitmap(gdiplusBitmap.get());
}

wil::unique_hbitmap IconResourceLoader::RetrieveBitmapFromGdiplusBitmap(Gdiplus::Bitmap *gdiplusBitmap) const
{
	if (!gdiplusBitmap)
	{
		return nullptr;
	}

	wil::unique_hbitmap bitmap;
	Gdiplus::Color color(0, 0, 0);
	Gdiplus::Status status = gdiplusBitmap->GetHBITMAP(color, &bitmap);

	if (status != Gdiplus::Status::Ok)
	{
		return nullptr;
	}

	return bitmap;
}

wil::unique_hicon IconResourceLoader::LoadIconFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGForDpi(icon, iconWidth, iconHeight, dpi);
	return RetrieveIconFromGdiplusBitmap(gdiplusBitmap.get());
}

wil::unique_hicon IconResourceLoader::LoadIconFromPNGAndScale(Icon icon, int iconWidth, int iconHeight) const
{
	auto gdiplusBitmap = LoadGdiplusBitmapFromPNGAndScale(icon, iconWidth, iconHeight);
	return RetrieveIconFromGdiplusBitmap(gdiplusBitmap.get());
}

wil::unique_hicon IconResourceLoader::RetrieveIconFromGdiplusBitmap(Gdiplus::Bitmap *gdiplusBitmap) const
{
	if (!gdiplusBitmap)
	{
		return nullptr;
	}

	wil::unique_hicon hicon;
	Gdiplus::Status status = gdiplusBitmap->GetHICON(&hicon);

	if (status != Gdiplus::Status::Ok)
	{
		return nullptr;
	}

	return hicon;
}

std::unique_ptr<Gdiplus::Bitmap> IconResourceLoader::LoadGdiplusBitmapFromPNGForDpi(Icon icon, int iconWidth, int iconHeight, int dpi) const
{
	int scaledIconWidth = MulDiv(iconWidth, dpi, USER_DEFAULT_SCREEN_DPI);
	int scaledIconHeight = MulDiv(iconHeight, dpi, USER_DEFAULT_SCREEN_DPI);
	return LoadGdiplusBitmapFromPNGAndScale(icon, scaledIconWidth, scaledIconHeight);
}

// This function is based on the steps performed by https://docs.microsoft.com/en-us/windows/win32/api/commctrl/nf-commctrl-loadiconmetric
// when loading an icon (see the remarks section on that page for details).
std::unique_ptr<Gdiplus::Bitmap> IconResourceLoader::LoadGdiplusBitmapFromPNGAndScale(Icon icon, int iconWidth, int iconHeight) const
{
	const IconMapping *mapping = nullptr;

	switch (m_iconTheme)
	{
	case IconTheme::Color:
		mapping = &ICON_RESOURCE_MAPPINGS_COLOR;
		break;

	case IconTheme::Windows10:
		mapping = &ICON_RESOURCE_MAPPINGS_WINDOWS_10;
		break;
	}

	const auto &iconSizeMappins = mapping->at(icon);

	auto match = std::find_if(iconSizeMappins.begin(), iconSizeMappins.end(),
		[iconWidth, iconHeight] (auto entry) {
			return iconWidth <= entry.first && iconHeight <= entry.first;
		}
	);

	if (match == iconSizeMappins.end())
	{
		match = std::prev(iconSizeMappins.end());
	}

	auto bitmap = ImageHelper::LoadGdiplusBitmapFromPNG(GetModuleHandle(nullptr), match->second);
	assert(bitmap);

	// If the icon size matches exactly, it doesn't need to be scaled, so can be
	// returned immediately.
	if (match->first == iconWidth
		&& match->first == iconHeight)
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